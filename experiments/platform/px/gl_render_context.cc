#include "experiments/platform/px/gl_render_context.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <map>
#include <tuple>
#include <utility>
#include <vector>

#include "experiments/platform/px/px_gl.h"

#if defined(__APPLE__)
#include "experiments/platform/px/px_font_private.h"
#endif

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

#if defined(__APPLE__)
constexpr const char* kGlyphVertexShader =
#include "experiments/platform/px/gl_glyph_vert.glsl"
    ;

constexpr const char* kGlyphFragmentShader =
#include "experiments/platform/px/gl_glyph_frag.glsl"
    ;
#endif

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

#if defined(__APPLE__)

struct glyph_atlas_key {
    const px_font_t* font = nullptr;
    uint32_t glyph = 0;
    int phase = 0;
    int scale_percent = 100;

    auto operator<=>(const glyph_atlas_key&) const = default;
};

struct glyph_atlas_placement {
    float u = 0.0f;
    float v = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    int color_page = -1;
    bool colored = false;
};

struct glyph_instance {
    float x, y, width, height;
    float u, v, uv_width, uv_height;
    float r, g, b, a;
    float colored, pad0, pad1, pad2;
};

class gl_text_render_state {
public:
    void request_reset() { reset_requested_.store(true, std::memory_order_release); }

    void begin_batch(vec2 viewport) {
        if (batch_depth_++ == 0) {
            batch_viewport_ = viewport;
            batch_groups_.clear();
        }
    }

    void end_batch() {
        if (batch_depth_ == 0 || --batch_depth_ != 0) {
            return;
        }
        flush_batch();
    }

    void finish_batch() {
        if (batch_depth_ == 0) {
            return;
        }
        batch_depth_ = 0;
        flush_batch();
    }

    void draw(px_font_t* font,
              const fx_layout& layout,
              vec2 origin,
              fcolor color,
              vec2 translation,
              vec2 scale,
              vec2 viewport,
              bool subpixel_positioning) {
        ensure_initialized();
        if (!program_ || !font || !font->font || layout.glyphs.empty()) {
            return;
        }
        if (reset_requested_.exchange(false, std::memory_order_acq_rel)) {
            placements_.clear();
            color_page_for_glyph_.clear();
            color_pages_.clear();
            atlas_row_x_ = 0;
            atlas_row_y_ = 0;
            atlas_row_height_ = 0;
            glBindTexture(GL_TEXTURE_2D, atlas_);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, kAtlasSize, kAtlasSize, 0, GL_BGRA,
                         GL_UNSIGNED_BYTE, nullptr);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        const float raster_scale = std::max(0.01f, static_cast<float>(std::abs(scale.x)));
        const int scale_percent = static_cast<int>(raster_scale * 100.0f + 0.5f);
        fx_glyph_cache& cache = font->glyph_cache(raster_scale);
        std::vector<glyph_instance> instances;
        instances.reserve(layout.glyphs.size());
        const double device_origin_x = translation.x + origin.x * scale.x;
        const double device_origin_y = translation.y + origin.y * scale.y;

        for (const fx_glyph& glyph : layout.glyphs) {
            const double x = device_origin_x + static_cast<double>(glyph.x_offset) * scale.x;
            const double y = device_origin_y + static_cast<double>(glyph.y_offset) * scale.y;
            const int device_x = static_cast<int>(std::floor(x));
            const double fraction = x - device_x;
            const int phase =
                subpixel_positioning ? std::clamp(static_cast<int>(fraction * 6.0), 0, 5) : 0;
            const glyph_atlas_key key{font, glyph.id, phase, scale_percent};
            const fx_glyph_cache::glyph_data& glyph_data =
                cache.lookup_glyph_data(glyph.id, static_cast<unsigned>(phase), false);
            if (glyph_data.bitmap.empty()) {
                continue;
            }
            if (glyph_data.bitmap.colored) {
                ensure_color_phase_pages(key, cache);
            }
            const glyph_atlas_placement* placement = place(key, glyph_data.bitmap);
            if (!placement) {
                continue;
            }

            const float instance_x = static_cast<float>(device_x + glyph_data.bitmap.bearing_x);
            // ST rounds in OpenGL's bottom-left coordinate system with frinta (nearest, ties away
            // from zero). Expressed in this renderer's top-left coordinates, the same operation
            // is nearest with half-pixel ties toward the top.
            const float instance_y =
                static_cast<float>(std::ceil(y - 0.5) + glyph_data.bitmap.bearing_y);

            instances.push_back({
                .x = instance_x,
                .y = instance_y,
                .width = placement->width,
                .height = placement->height,
                .u = placement->u,
                .v = placement->v,
                .uv_width = placement->width,
                .uv_height = placement->height,
                .r = color.r,
                .g = color.g,
                .b = color.b,
                .a = color.a,
                .colored = placement->colored ? 1.0f : 0.0f,
                .pad0 = 0.0f,
                .pad1 = 0.0f,
                .pad2 = 0.0f,
            });
            if (batch_depth_ != 0 && placement->colored) {
                add_to_batch(placement->color_page, instances.back());
                instances.pop_back();
            }
        }

        if (instances.empty()) {
            return;
        }

        render_instances(instances, viewport);
    }

private:
    struct color_page {
        int row_x = 0;
        int row_y = 0;
        int row_height = 0;
    };

    struct batch_group {
        int color_page = -1;
        std::vector<glyph_instance> instances;
    };

    void add_to_batch(int color_page, const glyph_instance& instance) {
        auto found = std::find_if(
            batch_groups_.begin(), batch_groups_.end(),
            [color_page](const batch_group& group) { return group.color_page == color_page; });
        if (found == batch_groups_.end()) {
            batch_groups_.push_back({.color_page = color_page});
            found = batch_groups_.end() - 1;
        }
        found->instances.push_back(instance);
    }

    void flush_batch() {
        for (const batch_group& group : batch_groups_) {
            render_instances(group.instances, batch_viewport_);
        }
        batch_groups_.clear();
    }

    void render_instances(const std::vector<glyph_instance>& instances, vec2 viewport) {
        if (instances.empty()) {
            return;
        }

        glUseProgram(program_);
        glUniform2f(viewport_uniform_, static_cast<float>(viewport.x),
                    static_cast<float>(viewport.y));
        glUniform1f(texture_size_uniform_, static_cast<float>(kAtlasSize));

        glActiveTexture(GL_TEXTURE0);
        glBindBuffer(GL_TEXTURE_BUFFER, instance_buffer_);
        glBufferData(GL_TEXTURE_BUFFER,
                     static_cast<GLsizeiptr>(instances.size() * sizeof(glyph_instance)),
                     instances.data(), GL_STREAM_DRAW);
        glBindTexture(GL_TEXTURE_BUFFER, instance_texture_);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, instance_buffer_);
        glUniform1i(instances_uniform_, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, atlas_);
        glUniform1i(atlas_uniform_, 1);

        glBindVertexArray(vao_);
        size_t first = 0;
        while (first < instances.size()) {
            const bool colored = instances[first].colored > 0.5f;
            size_t last = first + 1;
            while (last < instances.size() && (instances[last].colored > 0.5f) == colored) {
                ++last;
            }
            if (colored) {
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            } else {
                glBlendFuncSeparate(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR, GL_ONE,
                                    GL_ONE_MINUS_SRC1_ALPHA);
            }
            glUniform1i(instance_offset_uniform_, static_cast<GLint>(first));
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(last - first));
            first = last;
        }
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_BUFFER, 0);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
    }

    static constexpr int kAtlasSize = 4096;
    static constexpr int kColorPageSize = 256;

    int allocate_color_page(int width, int height) {
        if (width <= 0 || height <= 0 || width > kColorPageSize || height > kColorPageSize) {
            return -1;
        }
        if (color_pages_.empty()) {
            color_pages_.push_back({});
        }
        color_page* page = &color_pages_.back();
        if (page->row_x + width > kColorPageSize) {
            page->row_y += page->row_height;
            page->row_x = 0;
            page->row_height = 0;
        }
        if (page->row_y + height > kColorPageSize) {
            color_pages_.push_back({});
            page = &color_pages_.back();
        }
        const int page_index = static_cast<int>(color_pages_.size() - 1);
        page->row_x += width;
        page->row_height = std::max(page->row_height, height);
        return page_index;
    }

    void ensure_color_phase_pages(const glyph_atlas_key& requested_key, fx_glyph_cache& cache) {
        glyph_atlas_key first_phase = requested_key;
        first_phase.phase = 0;
        if (color_page_for_glyph_.contains(first_phase)) {
            return;
        }

        // A gl_glyph_cache miss in ST rasterizes and uploads all six horizontal subpixel phases.
        // Every variant occupies an atlas slot and can cross a texture-page boundary, which
        // controls batch draw order.
        for (int phase = 0; phase < 6; ++phase) {
            glyph_atlas_key phase_key = requested_key;
            phase_key.phase = phase;
            const fx_glyph_bitmap& bitmap =
                cache.lookup_glyph_data(requested_key.glyph, static_cast<unsigned>(phase), false)
                    .bitmap;
            color_page_for_glyph_.emplace(phase_key,
                                          allocate_color_page(static_cast<int>(bitmap.width),
                                                              static_cast<int>(bitmap.height)));
        }
    }

    void ensure_initialized() {
        if (initialized_) {
            return;
        }
        initialized_ = true;

        const GLuint vertex = compile_shader(GL_VERTEX_SHADER, kGlyphVertexShader);
        const GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, kGlyphFragmentShader);
        if (!vertex || !fragment) {
            if (vertex) {
                glDeleteShader(vertex);
            }
            if (fragment) {
                glDeleteShader(fragment);
            }
            return;
        }
        program_ = glCreateProgram();
        glAttachShader(program_, vertex);
        glAttachShader(program_, fragment);
        glBindFragDataLocationIndexed(program_, 0, 0, "frag_color");
        glBindFragDataLocationIndexed(program_, 0, 1, "frag_coverage");
        glLinkProgram(program_);
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        GLint linked = GL_FALSE;
        glGetProgramiv(program_, GL_LINK_STATUS, &linked);
        if (!linked) {
            char log[1024] = {};
            glGetProgramInfoLog(program_, sizeof(log), nullptr, log);
            std::fprintf(stderr, "px: glyph program link failed: %s\n", log);
            glDeleteProgram(program_);
            program_ = 0;
            return;
        }

        viewport_uniform_ = glGetUniformLocation(program_, "viewport");
        instances_uniform_ = glGetUniformLocation(program_, "instances");
        instance_offset_uniform_ = glGetUniformLocation(program_, "instance_offset");
        atlas_uniform_ = glGetUniformLocation(program_, "atlas");
        texture_size_uniform_ = glGetUniformLocation(program_, "texture_size");
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &instance_buffer_);
        glGenTextures(1, &instance_texture_);
        glGenTextures(1, &atlas_);
        glBindTexture(GL_TEXTURE_2D, atlas_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, kAtlasSize, kAtlasSize, 0, GL_BGRA,
                     GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    const glyph_atlas_placement* place(const glyph_atlas_key& key, const fx_glyph_bitmap& bitmap) {
        auto found = placements_.find(key);
        if (found != placements_.end()) {
            return &found->second;
        }

        const int width = static_cast<int>(bitmap.width);
        const int height = static_cast<int>(bitmap.height);
        if (width <= 0 || height <= 0 || width > kAtlasSize || height > kAtlasSize) {
            return nullptr;
        }

        int color_page = -1;
        if (bitmap.colored) {
            const auto found_page = color_page_for_glyph_.find(key);
            if (found_page != color_page_for_glyph_.end()) {
                color_page = found_page->second;
            }
        }

        if (atlas_row_x_ + width > kAtlasSize) {
            atlas_row_y_ += atlas_row_height_;
            atlas_row_x_ = 0;
            atlas_row_height_ = 0;
        }
        if (atlas_row_y_ + height > kAtlasSize) {
            return nullptr;
        }
        const int atlas_x = atlas_row_x_;
        const int atlas_y = atlas_row_y_;

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glBindTexture(GL_TEXTURE_2D, atlas_);
        // gl_texture_atlas::upload uses this exact format/type pair. The bitmap is already BGRA
        // byte order; using a packed integer type here needlessly makes component interpretation
        // depend on host integer layout.
        glTexSubImage2D(GL_TEXTURE_2D, 0, atlas_x, atlas_y, width, height, GL_BGRA,
                        GL_UNSIGNED_BYTE, bitmap.pixels.data());
        glBindTexture(GL_TEXTURE_2D, 0);

        glyph_atlas_placement placement{
            .u = static_cast<float>(atlas_x),
            .v = static_cast<float>(atlas_y),
            .width = static_cast<float>(width),
            .height = static_cast<float>(height),
            .color_page = color_page,
            .colored = bitmap.colored,
        };
        atlas_row_x_ += width;
        atlas_row_height_ = std::max(atlas_row_height_, height);
        return &placements_.emplace(key, placement).first->second;
    }

    bool initialized_ = false;
    GLuint program_ = 0;
    GLuint vao_ = 0;
    GLuint instance_buffer_ = 0;
    GLuint instance_texture_ = 0;
    GLuint atlas_ = 0;
    GLint viewport_uniform_ = -1;
    GLint instances_uniform_ = -1;
    GLint instance_offset_uniform_ = -1;
    GLint atlas_uniform_ = -1;
    GLint texture_size_uniform_ = -1;
    std::atomic<bool> reset_requested_ = false;
    std::map<glyph_atlas_key, glyph_atlas_placement> placements_;
    std::map<glyph_atlas_key, int> color_page_for_glyph_;
    std::vector<color_page> color_pages_;
    int atlas_row_x_ = 0;
    int atlas_row_y_ = 0;
    int atlas_row_height_ = 0;
    int batch_depth_ = 0;
    vec2 batch_viewport_;
    std::vector<batch_group> batch_groups_;
};

gl_text_render_state& text_render_state() {
    static auto* state = new gl_text_render_state;
    return *state;
}

#endif

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

void gl_render_context::draw_shaped_text(
    px_font_t* font, vec2 position, fcolor color, fx_layout* layout, bool subpixel_positioning) {
#if defined(__APPLE__)
    if (!layout || color.a <= 0.0f) {
        return;
    }
    if (rect_batch_) {
        rect_batch_->flush();
    }
    text_render_state().draw(font, *layout, position, color, translation_, scale_, device_size_,
                             subpixel_positioning);
#else
    (void)font;
    (void)position;
    (void)color;
    (void)layout;
    (void)subpixel_positioning;
#endif
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

void gl_render_context::begin_text_batch() {
#if defined(__APPLE__)
    if (rect_batch_) rect_batch_->flush();
    text_render_state().begin_batch(device_size_);
#endif
}

void gl_render_context::end_text_batch() {
#if defined(__APPLE__)
    text_render_state().end_batch();
#endif
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
#if defined(__APPLE__)
    text_render_state().finish_batch();
#endif
}

void gl_render_context::reset_glyph_atlas_for_testing() {
#if defined(__APPLE__)
    text_render_state().request_reset();
#endif
}
