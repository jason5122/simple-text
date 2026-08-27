#include "experiments/platform/px/gl_render_context.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>
#include <vector>

#include "experiments/platform/px/px_gl.h"

namespace {

constexpr int kMaximumDirtyRects = 128;

rect intersect_rect(rect a, rect b) {
    const double left = std::max(a.x, b.x);
    const double top = std::max(a.y, b.y);
    const double right = std::min(a.right(), b.right());
    const double bottom = std::min(a.bottom(), b.bottom());
    return rect{left, top, std::max(0.0, right - left), std::max(0.0, bottom - top)};
}

rect union_rect(rect a, rect b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    const double left = std::min(a.x, b.x);
    const double top = std::min(a.y, b.y);
    const double right = std::max(a.right(), b.right());
    const double bottom = std::max(a.bottom(), b.bottom());
    return rect{left, top, right - left, bottom - top};
}

recti intersect_recti(recti a, recti b) {
    recti result{std::max(a.left, b.left), std::max(a.top, b.top), std::min(a.right, b.right),
                 std::min(a.bottom, b.bottom)};
    if (result.empty()) {
        result.right = result.left;
        result.bottom = result.top;
    }
    return result;
}

struct rect_instance {
    float x;
    float y;
    float width;
    float height;
    float r;
    float g;
    float b;
    float a;
};

constexpr const char* kRectVertexShader =
#include "experiments/platform/px/gl_rect_vert.glsl"
    ;

constexpr const char* kRectFragmentShader =
#include "experiments/platform/px/gl_rect_frag.glsl"
    ;

GLuint compile_shader(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = {};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::fprintf(stderr, "px: gl_render_context shader compile failed: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// Persistent GL resources and the two alternating dynamic streams. ST has the same split between
// a process-wide g_gl_render_state and per-paint batch objects.
class gl_render_state {
public:
    bool usable() {
        ensure_initialized();
        return program_ != 0;
    }

    void draw_rects(const std::vector<rect_instance>& instances, vec2 viewport) {
        if (instances.empty() || !usable()) return;

        slot_ ^= 1;
        const size_t required = instances.size() * sizeof(rect_instance);

        glUseProgram(program_);
        glUniform2f(viewport_uniform_, static_cast<float>(viewport.x),
                    static_cast<float>(viewport.y));
        glUniform1i(instances_uniform_, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindBuffer(GL_TEXTURE_BUFFER, buffers_[slot_]);
        if (capacities_[slot_] < required) {
            capacities_[slot_] = std::max(required, capacities_[slot_] * 2);
            glBufferData(GL_TEXTURE_BUFFER, static_cast<GLsizeiptr>(capacities_[slot_]), nullptr,
                         GL_STREAM_DRAW);
            glBindTexture(GL_TEXTURE_BUFFER, textures_[slot_]);
            glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, buffers_[slot_]);
        }
        glBufferSubData(GL_TEXTURE_BUFFER, 0, static_cast<GLsizeiptr>(required), instances.data());
        glBindTexture(GL_TEXTURE_BUFFER, textures_[slot_]);
        glBindVertexArray(vao_);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(instances.size()));
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_BUFFER, 0);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
    }

private:
    void ensure_initialized() {
        if (initialized_) return;
        initialized_ = true;
        if (!px_gl_has_shaders()) return;

        const GLuint vertex = compile_shader(GL_VERTEX_SHADER, kRectVertexShader);
        const GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, kRectFragmentShader);
        if (!vertex || !fragment) {
            if (vertex) glDeleteShader(vertex);
            if (fragment) glDeleteShader(fragment);
            return;
        }

        program_ = glCreateProgram();
        glAttachShader(program_, vertex);
        glAttachShader(program_, fragment);
        glLinkProgram(program_);
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        GLint linked = GL_FALSE;
        glGetProgramiv(program_, GL_LINK_STATUS, &linked);
        if (!linked) {
            char log[1024] = {};
            glGetProgramInfoLog(program_, sizeof(log), nullptr, log);
            std::fprintf(stderr, "px: gl_render_context program link failed: %s\n", log);
            glDeleteProgram(program_);
            program_ = 0;
            return;
        }
        viewport_uniform_ = glGetUniformLocation(program_, "viewport");
        instances_uniform_ = glGetUniformLocation(program_, "instances");
        glGenVertexArrays(1, &vao_);
        glGenBuffers(2, buffers_);
        glGenTextures(2, textures_);
        glBindVertexArray(0);
    }

    bool initialized_ = false;
    GLuint program_ = 0;
    GLuint vao_ = 0;
    GLuint buffers_[2] = {};
    GLuint textures_[2] = {};
    size_t capacities_[2] = {};
    int slot_ = 0;
    GLint viewport_uniform_ = -1;
    GLint instances_uniform_ = -1;
};

gl_render_state& render_state() {
    // Intentionally process-lifetime. Destruction after the shared GL context has gone away would
    // be invalid, and ST's g_gl_render_state has the same lifetime.
    static gl_render_state* state = new gl_render_state;
    return *state;
}

}  // namespace

class gl_rect_batch {
public:
    explicit gl_rect_batch(vec2 viewport) : viewport_(viewport) {}
    ~gl_rect_batch() { flush(); }

    void add(rect area, fcolor color) {
        // ST blends premultiplied colors with ONE, ONE_MINUS_SRC_ALPHA.
        instances_.push_back(rect_instance{static_cast<float>(area.x), static_cast<float>(area.y),
                                           static_cast<float>(area.w), static_cast<float>(area.h),
                                           color.r * color.a, color.g * color.a, color.b * color.a,
                                           color.a});
    }

    void flush() {
        render_state().draw_rects(instances_, viewport_);
        instances_.clear();
    }

    int depth = 1;

private:
    vec2 viewport_;
    std::vector<rect_instance> instances_;
};

rect gl_render_context::normalize_dirty_rects(std::vector<rect>* dirty, rect window_bounds) {
    std::vector<rect> normalized;
    normalized.reserve(dirty->size());
    rect bounds;
    for (rect area : *dirty) {
        area = intersect_rect(area, window_bounds);
        if (area.empty()) continue;
        normalized.push_back(area);
        bounds = union_rect(bounds, area);
    }
    if (normalized.empty()) {
        normalized.push_back(window_bounds);
        bounds = window_bounds;
    }
    if (normalized.size() > kMaximumDirtyRects) {
        normalized.clear();
        normalized.push_back(bounds);
    }
    dirty->swap(normalized);
    return bounds;
}

gl_render_context::gl_render_context(
    vec2 device_size, double dpi_scale, const rect* dirty, int dirty_count, bool has_stencil)
    : device_size_(device_size),
      dpi_scale_(dpi_scale > 0.0 ? dpi_scale : 1.0),
      has_stencil_(has_stencil),
      scale_{dpi_scale_, dpi_scale_} {
    const rect full_bounds{0.0, 0.0, device_size.x / dpi_scale_, device_size.y / dpi_scale_};
    if (!dirty || dirty_count <= 0) {
        dirty = &full_bounds;
        dirty_count = 1;
    }
    for (int i = 0; i < dirty_count; ++i) paint_bounds_ = union_rect(paint_bounds_, dirty[i]);
    if (paint_bounds_.empty()) {
        paint_bounds_ = full_bounds;
        dirty = &full_bounds;
        dirty_count = 1;
    }
    clip_ = device_rect(paint_bounds_);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    if (has_stencil_) {
        establish_dirty_mask(dirty, dirty_count);
    } else {
        glDisable(GL_STENCIL_TEST);
    }
    glEnable(GL_SCISSOR_TEST);
    apply_clip();
}

gl_render_context::~gl_render_context() {
    finish();
    if (has_stencil_) {
        glStencilMask(0xff);
        glDisable(GL_STENCIL_TEST);
    }
    glDisable(GL_SCISSOR_TEST);
}

recti gl_render_context::device_rect(rect area) const {
    const rect transformed = transformed_rect(area);
    const double x0 = transformed.x;
    const double y0 = transformed.y;
    const double x1 = transformed.right();
    const double y1 = transformed.bottom();
    recti result{static_cast<int>(std::floor(std::min(x0, x1))),
                 static_cast<int>(std::floor(std::min(y0, y1))),
                 static_cast<int>(std::ceil(std::max(x0, x1))),
                 static_cast<int>(std::ceil(std::max(y0, y1)))};
    return intersect_recti(
        result, recti{0, 0, static_cast<int>(device_size_.x), static_cast<int>(device_size_.y)});
}

rect gl_render_context::transformed_rect(rect area) const {
    const double x0 = translation_.x + area.x * scale_.x;
    const double y0 = translation_.y + area.y * scale_.y;
    const double x1 = translation_.x + area.right() * scale_.x;
    const double y1 = translation_.y + area.bottom() * scale_.y;
    return rect{std::min(x0, x1), std::min(y0, y1), std::abs(x1 - x0), std::abs(y1 - y0)};
}

void gl_render_context::apply_clip() {
    glScissor(clip_.left, static_cast<int>(device_size_.y) - clip_.bottom,
              std::max(0, clip_.width()), std::max(0, clip_.height()));
}

void gl_render_context::establish_dirty_mask(const rect* dirty, int dirty_count) {
    glStencilMask(0xff);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);

    if (render_state().usable()) {
        // This is ST's path: rasterize all dirty rectangles in one instanced draw with color
        // writes disabled and REPLACE stencil operations. Scissor remains the coarser bounding
        // union used during ordinary drawing; the stencil retains the exact disjoint region.
        std::vector<rect_instance> instances;
        instances.reserve(static_cast<size_t>(dirty_count));
        for (int i = 0; i < dirty_count; ++i) {
            const rect area = transformed_rect(dirty[i]);
            if (area.empty()) continue;
            instances.push_back(rect_instance{
                static_cast<float>(area.x), static_cast<float>(area.y), static_cast<float>(area.w),
                static_cast<float>(area.h), 0.0f, 0.0f, 0.0f, 0.0f});
        }
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_ALWAYS, 1, 0xff);
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        render_state().draw_rects(instances, device_size_);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    } else {
        // A legacy Windows context may lack the shader entry points. Rectangle drawing will be
        // inert there, but retain a valid dirty stencil rather than leaving unrelated pixels
        // writable.
        glEnable(GL_SCISSOR_TEST);
        glClearStencil(1);
        for (int i = 0; i < dirty_count; ++i) {
            const recti area = device_rect(dirty[i]);
            if (area.empty()) continue;
            glScissor(area.left, static_cast<int>(device_size_.y) - area.bottom, area.width(),
                      area.height());
            glClear(GL_STENCIL_BUFFER_BIT);
        }
        glClearStencil(0);
    }
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 1, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0x00);
}

void gl_render_context::draw_rect(rect area, fill_mode fill) {
    const rect transformed = transformed_rect(area);
    if (transformed.empty() || fill.color.a <= 0.0f) return;
    if (rect_batch_) {
        rect_batch_->add(transformed, fill.color);
        return;
    }
    gl_rect_batch batch(device_size_);
    batch.add(transformed, fill.color);
}

void gl_render_context::translate(double x, double y) {
    translation_.x += x * scale_.x;
    translation_.y += y * scale_.y;
}

void gl_render_context::scale(double x, double y) {
    scale_.x *= x;
    scale_.y *= y;
}

void gl_render_context::restrict_clip_rect(rect area) {
    if (rect_batch_) rect_batch_->flush();
    clip_ = intersect_recti(clip_, device_rect(area));
    apply_clip();
}

void gl_render_context::push_state(bool preserve_batch) {
    saved_state state{translation_, scale_, clip_};
    if (!preserve_batch) {
        state.has_batch_state = true;
        state.rect_batch = std::move(rect_batch_);
    }
    state_stack_.push_back(std::move(state));
}

void gl_render_context::pop_state() {
    if (state_stack_.empty()) return;
    saved_state state = std::move(state_stack_.back());
    state_stack_.pop_back();
    if (state.has_batch_state) {
        // Submit child drawing before returning to the parent's batch, preserving draw order.
        rect_batch_.reset();
        rect_batch_ = std::move(state.rect_batch);
    } else if (rect_batch_) {
        // Instances queued under the child clip must be submitted before restoring the parent
        // clip.
        rect_batch_->flush();
    }
    translation_ = state.translation;
    scale_ = state.scale;
    clip_ = state.clip;
    apply_clip();
}

void gl_render_context::begin_rect_batch() {
    if (rect_batch_) {
        ++rect_batch_->depth;
        return;
    }
    rect_batch_ = std::make_unique<gl_rect_batch>(device_size_);
}

void gl_render_context::end_rect_batch() {
    if (!rect_batch_) return;
    if (--rect_batch_->depth == 0) rect_batch_.reset();
}

void gl_render_context::finish() {
    // pop_state() first submits child work under the child clip and then restores its parent
    // batch. Repeating that operation preserves ordering even for accidentally unbalanced state
    // scopes.
    while (!state_stack_.empty()) pop_state();
    rect_batch_.reset();
}
