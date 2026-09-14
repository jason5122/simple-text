#include "gl/opengl.h"
#include "gui/renderer/renderer.h"
#include "gui/renderer/texture_renderer.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <spdlog/spdlog.h>

using namespace gl;

namespace {
const std::string kVertexShader =
#include "gui/renderer/shaders/texture_vert.glsl"
    ;
const std::string kFragmentShader =
#include "gui/renderer/shaders/texture_frag.glsl"
    ;
}  // namespace

namespace gui {

TextureRenderer::TextureRenderer() : shader_program{kVertexShader, kFragmentShader} {
    constexpr GLuint indices[] = {
        0, 1, 3,  // First triangle.
        1, 2, 3,  // Second triangle.
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo_instance);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData) * kBatchMax, nullptr, GL_STREAM_DRAW);

    GLuint index = 0;

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, coords));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, glyph));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, uv));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, color));
    glVertexAttribDivisor(index++, 1);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

TextureRenderer::~TextureRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo_instance);
    glDeleteBuffers(1, &ebo);
}

TextureRenderer::TextureRenderer(TextureRenderer&& other) noexcept
    : shader_program{std::move(other.shader_program)},
      vao{other.vao},
      vbo_instance{other.vbo_instance},
      ebo{other.ebo} {
    other.vao = 0;
    other.vbo_instance = 0;
    other.ebo = 0;
}

TextureRenderer& TextureRenderer::operator=(TextureRenderer&& other) noexcept {
    if (&other != this) {
        shader_program = std::move(other.shader_program);
        vao = other.vao;
        vbo_instance = other.vbo_instance;
        ebo = other.ebo;
        other.vao = 0;
        other.vbo_instance = 0;
        other.ebo = 0;
    }
    return *this;
}

void TextureRenderer::add_line_layout(const editor::LineLayout& line_layout,
                                      const Point& coords,
                                      const Point& min_coords,
                                      const Point& max_coords,
                                      const std::function<Rgb(size_t)>& highlight_callback) {
    auto& renderer = Renderer::instance();
    auto& texture_cache = renderer.texture_cache();
    const auto metrics = renderer.font_cache().metrics(line_layout.font_id);

    // Everything below is in absolute device pixels, y-down. `coords` is the top-left corner of
    // the line box, so the alphabetic baseline sits `ascent` below it.
    const int baseline_y = coords.y + metrics.ascent;

    constexpr float kUvPerPixel = 1.0f / Atlas::kAtlasSize;

    for (const auto& glyph : line_layout.glyphs) {
        const auto& rglyph = texture_cache.get_glyph(line_layout.font_id, glyph.glyph_id);
        if (rglyph.width <= 0 || rglyph.height <= 0) continue;

        // The bitmap's top-left corner is the pen position offset by the glyph's bearing.
        const int left = coords.x + glyph.x + rglyph.bearing_x;
        const int top = baseline_y + glyph.y + rglyph.bearing_y;
        const int right = left + rglyph.width;
        const int bottom = top + rglyph.height;

        // Clip the bitmap to [min_coords, max_coords), trimming its UVs by the same amount.
        const int clipped_left = std::max(left, min_coords.x);
        const int clipped_top = std::max(top, min_coords.y);
        const int clipped_right = std::min(right, max_coords.x);
        const int clipped_bottom = std::min(bottom, max_coords.y);
        if (clipped_left >= clipped_right || clipped_top >= clipped_bottom) continue;

        const int width = clipped_right - clipped_left;
        const int height = clipped_bottom - clipped_top;
        const Vec4 uv = {
            .x = rglyph.uv.x + static_cast<float>(clipped_left - left) * kUvPerPixel,
            .y = rglyph.uv.y + static_cast<float>(clipped_top - top) * kUvPerPixel,
            .z = static_cast<float>(width) * kUvPerPixel,
            .w = static_cast<float>(height) * kUvPerPixel,
        };

        const uint8_t kind = rglyph.colored ? kColoredText : kMonochromeText;
        InstanceData instance = {
            .coords = {static_cast<float>(clipped_left), static_cast<float>(clipped_top)},
            .glyph = {0, 0, static_cast<float>(width), static_cast<float>(height)},
            .uv = uv,
            .color = Rgba::fromRgb(highlight_callback(glyph.index), kind),
        };
        insert_into_batch(rglyph.page, instance);
    }
}

void TextureRenderer::add_image(size_t image_index, const Point& coords, const Rgb& color) {
    const auto& texture_cache = Renderer::instance().texture_cache();
    const auto& image = texture_cache.get_image(image_index);
    InstanceData instance = {
        .coords = {static_cast<float>(coords.x), static_cast<float>(coords.y)},
        .glyph = {0, 0, static_cast<float>(image.size.width),
                  static_cast<float>(image.size.height)},
        .uv = image.uv,
        .color = Rgba::fromRgb(color, kPlainTexture),
    };
    insert_into_batch(image.page, std::move(instance));
}

void TextureRenderer::add_color_image(size_t image_index, const Point& coords) {
    const auto& texture_cache = Renderer::instance().texture_cache();
    const auto& image = texture_cache.get_image(image_index);
    InstanceData instance = {
        .coords = {static_cast<float>(coords.x), static_cast<float>(coords.y)},
        .glyph = {0, 0, static_cast<float>(image.size.width),
                  static_cast<float>(image.size.height)},
        .uv = image.uv,
        .color = {.a = kColoredImage},
    };
    insert_into_batch(image.page, std::move(instance));
}

void TextureRenderer::flush(const Size& screen_size) {
    const auto& texture_cache = Renderer::instance().texture_cache();

    glBlendFuncSeparate(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR, GL_ZERO, GL_ONE);

    GLuint shader_id = shader_program.id();
    glUseProgram(shader_id);
    glUniform2f(glGetUniformLocation(shader_id, "resolution"), screen_size.width,
                screen_size.height);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);

    for (size_t page = 0; page < texture_cache.pages().size(); ++page) {
        // TODO: Refactor this ugly hack.
        while (batches.size() <= page) {
            batches.emplace_back();
            batches.back().reserve(kBatchMax);
        }

        std::vector<InstanceData>& batch = batches.at(page);
        if (batch.empty()) {
            continue;
        }

        GLuint batch_tex = texture_cache.pages().at(page).tex();
        glBindTexture(GL_TEXTURE_2D, batch_tex);

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * batch.size(), batch.data());
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, batch.size());

        batch.clear();
    }

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextureRenderer::render_atlas_page(size_t page,
                                        const Point& coords,
                                        const Point& min_coords,
                                        const Point& max_coords) {
    int x = coords.x;
    int y = coords.y;
    int width = Atlas::kAtlasSize;
    int height = Atlas::kAtlasSize;

    float uv_x = 0;
    float uv_y = 0;
    float uv_width = 1;
    float uv_height = 1;

    int left_edge = x;
    int right_edge = left_edge + width;
    int top_edge = y;
    int bottom_edge = top_edge + height;

    if (right_edge <= min_coords.x) return;
    if (left_edge > max_coords.x) return;
    if (bottom_edge <= min_coords.y) return;
    if (top_edge > max_coords.y) return;

    if (left_edge < min_coords.x) {
        int diff = min_coords.x - left_edge;
        float uv_diff = static_cast<float>(diff) / Atlas::kAtlasSize;
        width -= diff;
        uv_width -= uv_diff;
        x += diff;
        uv_x += uv_diff;
    }
    if (right_edge > max_coords.x) {
        int diff = right_edge - max_coords.x;
        float uv_diff = static_cast<float>(diff) / Atlas::kAtlasSize;
        width -= diff;
        uv_width -= uv_diff;
    }
    if (top_edge < min_coords.y) {
        int diff = min_coords.y - top_edge;
        float uv_diff = static_cast<float>(diff) / Atlas::kAtlasSize;
        height -= diff;
        uv_height -= uv_diff;
        y += diff;
        uv_y += uv_diff;
    }
    if (bottom_edge > max_coords.y) {
        int diff = bottom_edge - max_coords.y;
        float uv_diff = static_cast<float>(diff) / Atlas::kAtlasSize;
        height -= diff;
        uv_height -= uv_diff;
    }

    // TODO: Remove this.
    if (width <= 0 || height <= 0) {
        return;
    }

    InstanceData instance{
        .coords = {static_cast<float>(x), static_cast<float>(y)},
        .glyph = {0, 0, static_cast<float>(width), static_cast<float>(height)},
        .uv = {uv_x, uv_y, uv_width, uv_height},
        .color = {255, 255, 255, true},
    };
    insert_into_batch(page, std::move(instance));
}

void TextureRenderer::insert_into_batch(size_t page, const InstanceData& instance) {
    // TODO: Refactor this ugly hack.
    while (batches.size() <= page) {
        batches.emplace_back();
        batches.back().reserve(kBatchMax);
    }

    std::vector<InstanceData>& batch = batches.at(page);

    batch.emplace_back(std::move(instance));
    if (batch.size() == kBatchMax) {
        spdlog::info("TextureRenderer error: attempted to insert into a full batch!");
    }
}

}  // namespace gui
