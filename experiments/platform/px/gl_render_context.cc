#include "experiments/platform/px/gl_render_context.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdio>
#include <map>
#include <tuple>
#include <utility>
#include <vector>

#include "experiments/platform/px/px_gl.h"

#if defined(__APPLE__) || defined(_WIN32)
#include "experiments/platform/px/px_font_internal.h"
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

#if defined(__APPLE__) || defined(_WIN32)
constexpr const char* kGlyphVertexShader =
#include "experiments/platform/px/gl_glyph_vert.glsl"
    ;

constexpr const char* kGlyphFragmentShader =
#include "experiments/platform/px/gl_glyph_frag.glsl"
    ;
#endif

GLuint compile_shader_sources(GLenum type, const char* const* sources, GLsizei source_count) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, source_count, sources, nullptr);
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

GLuint compile_shader(GLenum type, const char* source) {
    return compile_shader_sources(type, &source, 1);
}

GLuint link_program(GLuint vertex, GLuint fragment, const char* name) {
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[1024] = {};
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::fprintf(stderr, "px: %s program link failed: %s\n", name, log);
        glDeleteProgram(program);
        return 0;
    }
    return program;
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

#if defined(__APPLE__) || defined(_WIN32)

struct glyph_atlas_key {
    const px_font_t* font = nullptr;
    uint32_t glyph = 0;
    int phase = 0;
    uint32_t scale_percent = 100;
    bool alternate = false;

    auto operator<=>(const glyph_atlas_key&) const = default;
};

struct glyph_atlas_placement {
    float u = 0.0f;
    float v = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    int page = -1;
    bool colored = false;
};

struct glyph_instance_data {
    float r, g, b, a;
    float x, y, width, height;
    // The final lanes hold fade or horizontal-clip bounds in those shader variants.
    float u, v, effect_start, effect_end;
};
static_assert(sizeof(glyph_instance_data) == 3 * sizeof(float) * 4);

struct glyph_program {
    GLuint id = 0;
    GLint viewport_uniform = -1;
    GLint instances_uniform = -1;
    GLint instance_offset_uniform = -1;
    GLint tex_uniform = -1;
    GLint texture_size_uniform = -1;
    GLint colored_uniform = -1;
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

    void flush_pending_batch() {
        if (batch_depth_ != 0) {
            flush_batch();
        }
    }

    void draw(px_font_t* font,
              const fx_layout& layout,
              vec2 origin,
              fcolor color,
              vec2 translation,
              vec2 scale,
              vec2 viewport,
              recti clip,
              bool subpixel_positioning) {
        ensure_initialized();
        if (!program_.id || !font || !font->font || layout.glyphs.empty() || clip.empty()) {
            return;
        }
        if (reset_requested_.exchange(false, std::memory_order_acq_rel)) {
            flush_batch();
            for (auto& [unused, atlas] : atlas_sets_) {
                atlas.placements.clear();
                atlas.active_page_count = atlas.pages.empty() ? 0 : 1;
                for (atlas_page& page : atlas.pages) {
                    page.row_x = 0;
                    page.row_y = 0;
                    page.row_height = 0;
                    glBindTexture(GL_TEXTURE_2D, page.texture);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlas.size, atlas.size, 0, GL_BGRA,
                                 GL_UNSIGNED_BYTE, nullptr);
                }
            }
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        const float raster_scale = std::max(0.01f, static_cast<float>(std::abs(scale.x)));
        const uint32_t scale_percent =
            static_cast<uint32_t>(static_cast<double>(raster_scale) * 100.0);
        fx_glyph_cache& cache = font->glyph_cache(raster_scale);
        atlas_set& atlas = atlas_sets_[{font, scale_percent}];
        if (atlas.size == 0) atlas.size = atlas_size_for(*font, raster_scale);
        std::vector<texture_batch_group> immediate_groups;
        std::vector<texture_batch_group>& groups =
            batch_depth_ != 0 ? batch_groups_ : immediate_groups;
        const double device_origin_x = translation.x + origin.x * scale.x;
        const double device_origin_y = translation.y + origin.y * scale.y;
        const float lightness =
            (std::max({color.r, color.g, color.b}) + std::min({color.r, color.g, color.b})) * 0.5f;
        // Sublime selects the inverted glyph-cache polarity only for very light tints
        // (ucomiss against 0.75 at 0x1402cc583).  The comparison is `lightness > 0.75`;
        // reversing the operands makes ordinary black text render as solid glyph tiles.
        const bool alternate = lightness > 0.75f;

        for (const fx_glyph& glyph : layout.glyphs) {
            const double x = device_origin_x + static_cast<double>(glyph.x_offset) * scale.x;
            const double y = device_origin_y + static_cast<double>(glyph.y_offset) * scale.y;
            const int device_x = static_cast<int>(std::floor(x));
            const double fraction = x - std::floor(x);
            const int phase =
                subpixel_positioning ? std::clamp(static_cast<int>(fraction * 6.0), 0, 5) : 0;
            const glyph_atlas_key key{font, glyph.id, phase, scale_percent, alternate};
            ensure_phase_pages(&atlas, key, cache);
            const fx_glyph_bitmap& bitmap =
                cache.lookup_glyph_data(glyph.id, static_cast<unsigned>(phase), alternate);
            if (bitmap.empty()) {
                continue;
            }
            const glyph_atlas_placement* placement = place(&atlas, key, bitmap);
            if (!placement) {
                continue;
            }

            const float instance_x = static_cast<float>(device_x + bitmap.bearing_x);
            // ST rounds in OpenGL's bottom-left coordinate system with frinta (nearest, ties away
            // from zero). Expressed in this renderer's top-left coordinates, the same operation
            // is nearest with half-pixel ties toward the top.
            const float instance_y = static_cast<float>(std::ceil(y - 0.5) + bitmap.bearing_y);
            const float instance_width = static_cast<float>(bitmap.width);
            const float instance_height = static_cast<float>(bitmap.height);
            if (instance_x + instance_width <= static_cast<float>(clip.left) ||
                instance_x >= static_cast<float>(clip.right) ||
                instance_y + instance_height <= static_cast<float>(clip.top) ||
                instance_y >= static_cast<float>(clip.bottom)) {
                continue;
            }

            const glyph_instance_data instance{
                .r = color.r,
                .g = color.g,
                .b = color.b,
                .a = color.a,
                .x = instance_x,
                .y = instance_y,
                .width = placement->width,
                .height = placement->height,
                .u = placement->u,
                .v = placement->v,
                .effect_start = 0.0f,
                .effect_end = 0.0f,
            };
            add_to_groups(&groups, &atlas, placement->page, placement->colored, instance);
        }

        if (batch_depth_ == 0) {
            render_groups(immediate_groups, viewport);
        }
    }

private:
    struct atlas_page {
        GLuint texture = 0;
        int row_x = 0;
        int row_y = 0;
        int row_height = 0;
    };

    struct atlas_set {
        int size = 0;
        std::map<glyph_atlas_key, glyph_atlas_placement> placements;
        std::vector<atlas_page> pages;
        size_t active_page_count = 0;
    };

    struct texture_batch_group {
        const atlas_set* atlas = nullptr;
        int page = -1;
        bool colored = false;
        std::vector<glyph_instance_data> instances;
    };

    static void add_to_groups(std::vector<texture_batch_group>* groups,
                              const atlas_set* atlas,
                              int page,
                              bool colored,
                              glyph_instance_data instance) {
        auto found = std::find_if(groups->begin(), groups->end(),
                                  [atlas, page, colored](const texture_batch_group& group) {
                                      return group.atlas == atlas && group.page == page &&
                                             group.colored == colored;
                                  });
        if (found == groups->end()) {
            groups->push_back({.atlas = atlas, .page = page, .colored = colored});
            found = std::prev(groups->end());
        }
        found->instances.push_back(instance);
    }

    void flush_batch() {
        render_groups(batch_groups_, batch_viewport_);
        for (texture_batch_group& group : batch_groups_) group.instances.clear();
    }

    void render_groups(const std::vector<texture_batch_group>& groups, vec2 viewport) {
        size_t instance_count = 0;
        for (const texture_batch_group& group : groups) {
            instance_count += group.instances.size();
        }
        if (instance_count == 0) return;

        begin_render(instance_count, viewport);
        size_t first = 0;
        for (const texture_batch_group& group : groups) {
            upload_instances(group.instances, first);
            first += group.instances.size();
        }
        first = 0;
        for (const texture_batch_group& group : groups) {
            draw_instances(first, group.instances.size(), group.atlas, group.page, group.colored);
            first += group.instances.size();
        }
        end_render();
    }

    void begin_render(size_t instance_count, vec2 viewport) {
        instance_slot_ ^= 1;
        const size_t required = instance_count * sizeof(glyph_instance_data);
        glActiveTexture(GL_TEXTURE0);
        glBindBuffer(GL_TEXTURE_BUFFER, instance_buffers_[instance_slot_]);
        if (instance_capacities_[instance_slot_] < required) {
            instance_capacities_[instance_slot_] =
                std::max(required, instance_capacities_[instance_slot_] * 2);
            glBufferData(GL_TEXTURE_BUFFER,
                         static_cast<GLsizeiptr>(instance_capacities_[instance_slot_]), nullptr,
                         GL_STREAM_DRAW);
            glBindTexture(GL_TEXTURE_BUFFER, instance_textures_[instance_slot_]);
            glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, instance_buffers_[instance_slot_]);
        }
        glBindTexture(GL_TEXTURE_BUFFER, instance_textures_[instance_slot_]);

        glBindVertexArray(vao_);
        glUseProgram(program_.id);
        glUniform2f(program_.viewport_uniform, static_cast<float>(viewport.x),
                    static_cast<float>(viewport.y));
        glUniform1i(program_.instances_uniform, 0);
        glUniform1i(program_.tex_uniform, 1);
    }

    static void upload_instances(const std::vector<glyph_instance_data>& instances, size_t first) {
        if (instances.empty()) {
            return;
        }
        glBufferSubData(GL_TEXTURE_BUFFER,
                        static_cast<GLintptr>(first * sizeof(glyph_instance_data)),
                        static_cast<GLsizeiptr>(instances.size() * sizeof(glyph_instance_data)),
                        instances.data());
    }

    void draw_instances(
        size_t first, size_t count, const atlas_set* atlas, int page, bool colored) {
        if (count == 0) {
            return;
        }
        if (!atlas || page < 0 || static_cast<size_t>(page) >= atlas->active_page_count) return;
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, atlas->pages[static_cast<size_t>(page)].texture);
        glUniform1i(program_.instance_offset_uniform, static_cast<GLint>(first));
        glUniform1f(program_.texture_size_uniform, static_cast<float>(atlas->size));
        glUniform1i(program_.colored_uniform, colored ? 1 : 0);
        if (colored) {
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
        }
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(count));
    }

    static void end_render() {
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_BUFFER, 0);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
    }

    static int atlas_size_for(const px_font_t& font, float scale) {
        const float line_height = font.font->metrics().line_height;
        const unsigned target =
            std::max(1u, static_cast<unsigned>(std::ceil(line_height * scale * 8.0f)));
        return static_cast<int>(std::bit_ceil(target));
    }

    void ensure_phase_pages(atlas_set* atlas,
                            const glyph_atlas_key& requested_key,
                            fx_glyph_cache& cache) {
        glyph_atlas_key first_phase = requested_key;
        first_phase.phase = 0;
        if (atlas->placements.contains(first_phase)) return;

        // A gl_glyph_cache miss in ST rasterizes and uploads all six horizontal subpixel phases.
        // Atlas-page assignment intentionally controls the draw order when glyphs overlap.
        for (int phase = 0; phase < 6; ++phase) {
            glyph_atlas_key phase_key = requested_key;
            phase_key.phase = phase;
            const fx_glyph_bitmap& bitmap = cache.lookup_glyph_data(
                requested_key.glyph, static_cast<unsigned>(phase), requested_key.alternate);
            if (!bitmap.empty()) place(atlas, phase_key, bitmap);
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
        program_.id = link_program(vertex, fragment, "glyph");
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (!program_.id) {
            return;
        }

        program_.viewport_uniform = glGetUniformLocation(program_.id, "viewport");
        program_.instances_uniform = glGetUniformLocation(program_.id, "instances");
        program_.instance_offset_uniform = glGetUniformLocation(program_.id, "instance_offset");
        program_.tex_uniform = glGetUniformLocation(program_.id, "atlas");
        program_.texture_size_uniform = glGetUniformLocation(program_.id, "texture_size");
        program_.colored_uniform = glGetUniformLocation(program_.id, "colored");
        glGenVertexArrays(1, &vao_);
        glGenBuffers(2, instance_buffers_);
        glGenTextures(2, instance_textures_);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    static atlas_page* activate_page(atlas_set* atlas) {
        if (atlas->active_page_count < atlas->pages.size()) {
            atlas_page& page = atlas->pages[atlas->active_page_count++];
            page.row_x = 0;
            page.row_y = 0;
            page.row_height = 0;
            return &page;
        }

        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlas->size, atlas->size, 0, GL_BGRA,
                     GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        atlas->pages.push_back({.texture = texture});
        ++atlas->active_page_count;
        return &atlas->pages.back();
    }

    static const glyph_atlas_placement* place(atlas_set* atlas,
                                              const glyph_atlas_key& key,
                                              const fx_glyph_bitmap& bitmap) {
        auto found = atlas->placements.find(key);
        if (found != atlas->placements.end()) {
            return &found->second;
        }

        const int width = static_cast<int>(bitmap.width);
        const int height = static_cast<int>(bitmap.height);
        if (width <= 0 || height <= 0 || width > atlas->size || height > atlas->size) {
            return nullptr;
        }

        atlas_page* page = atlas->active_page_count == 0
                               ? activate_page(atlas)
                               : &atlas->pages[atlas->active_page_count - 1];
        if (page->row_x + width > atlas->size) {
            page->row_y += page->row_height;
            page->row_x = 0;
            page->row_height = 0;
        }
        if (page->row_y + height > atlas->size) {
            page = activate_page(atlas);
        }
        const int atlas_x = page->row_x;
        const int atlas_y = page->row_y;
        const int page_index = static_cast<int>(atlas->active_page_count - 1);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glBindTexture(GL_TEXTURE_2D, page->texture);
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
            .page = page_index,
            .colored = bitmap.colored,
        };
        page->row_x += width;
        page->row_height = std::max(page->row_height, height);
        return &atlas->placements.emplace(key, placement).first->second;
    }

    bool initialized_ = false;
    glyph_program program_;
    GLuint vao_ = 0;
    GLuint instance_buffers_[2] = {};
    GLuint instance_textures_[2] = {};
    size_t instance_capacities_[2] = {};
    int instance_slot_ = 0;
    std::atomic<bool> reset_requested_ = false;
    std::map<std::pair<const px_font_t*, uint32_t>, atlas_set> atlas_sets_;
    int batch_depth_ = 0;
    vec2 batch_viewport_;
    std::vector<texture_batch_group> batch_groups_;
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
    const fcolor normalized = fill.color;
    if (transformed.empty() || normalized.a <= 0.0f) return;
#if defined(__APPLE__) || defined(_WIN32)
    text_render_state().flush_pending_batch();
#endif
    if (rect_batch_) {
        rect_batch_->add(transformed, normalized);
        return;
    }
    gl_rect_batch batch(device_size_);
    batch.add(transformed, normalized);
}

void gl_render_context::draw_shaped_text(
    px_font_t* font, vec2 position, color value, fx_layout* layout, bool subpixel_positioning) {
#if defined(__APPLE__) || defined(_WIN32)
    const fcolor normalized = value;
    if (!layout || normalized.a <= 0.0f) {
        return;
    }
    if (rect_batch_) {
        rect_batch_->flush();
    }
    text_render_state().draw(font, *layout, position, normalized, translation_, scale_,
                             device_size_, clip_, subpixel_positioning);
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
#if defined(__APPLE__) || defined(_WIN32)
    text_render_state().flush_pending_batch();
#endif
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
#if defined(__APPLE__) || defined(_WIN32)
    // Glyph instances do not carry a clip rectangle. Submit child text while the child scissor is
    // still active, before restoring the parent state below.
    text_render_state().flush_pending_batch();
#endif
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
#if defined(__APPLE__) || defined(_WIN32)
    text_render_state().flush_pending_batch();
#endif
    if (rect_batch_) {
        ++rect_batch_->depth;
        return;
    }
    rect_batch_ = std::make_unique<gl_rect_batch>(device_size_);
}

void gl_render_context::begin_text_batch() {
#if defined(__APPLE__) || defined(_WIN32)
    if (rect_batch_) rect_batch_->flush();
    text_render_state().begin_batch(device_size_);
#endif
}

void gl_render_context::end_text_batch() {
#if defined(__APPLE__) || defined(_WIN32)
    text_render_state().end_batch();
#endif
}

void gl_render_context::end_rect_batch() {
    if (!rect_batch_) return;
    if (--rect_batch_->depth == 0) rect_batch_.reset();
}

void gl_render_context::begin_line_batch() {}

void gl_render_context::end_line_batch() {}

void gl_render_context::finish() {
    // pop_state() first submits child work under the child clip and then restores its parent
    // batch. Repeating that operation preserves ordering even for accidentally unbalanced state
    // scopes.
    while (!state_stack_.empty()) pop_state();
    rect_batch_.reset();
#if defined(__APPLE__) || defined(_WIN32)
    text_render_state().finish_batch();
#endif
}

void gl_render_context::reset_glyph_atlas_for_testing() {
#if defined(__APPLE__) || defined(_WIN32)
    text_render_state().request_reset();
#endif
}
