#include "text_renderer.h"

#include "gui/renderer/renderer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "opengl/gl.h"
using namespace opengl;

namespace {

const std::string kVertexShader =
#include "gui/renderer/shaders/text_vert.glsl"
    ;
const std::string kFragmentShader =
#include "gui/renderer/shaders/text_frag.glsl"
    ;

}  // namespace

// TODO: Debug; remove this.
#include "util/profile_util.h"
#include <cassert>
#include <fmt/base.h>

namespace gui {

TextRenderer::TextRenderer() : shader_program{kVertexShader, kFragmentShader} {
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

TextRenderer::~TextRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo_instance);
    glDeleteBuffers(1, &ebo);
}

TextRenderer::TextRenderer(TextRenderer&& other)
    : shader_program{std::move(other.shader_program)},
      vao{other.vao},
      vbo_instance{other.vbo_instance},
      ebo{other.ebo} {
    other.vao = 0;
    other.vbo_instance = 0;
    other.ebo = 0;
}

TextRenderer& TextRenderer::operator=(TextRenderer&& other) {
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

void TextRenderer::renderLineLayout(const font::LineLayout& line_layout,
                                    const app::Point& coords,
                                    const std::function<Rgb(size_t)>& highlight_callback,
                                    int min_x,
                                    int max_x,
                                    int min_y,
                                    int max_y) {
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(line_layout.layout_font_id);
    int line_height = metrics.line_height;

    auto& glyph_cache = Renderer::instance().getGlyphCache();

    // TODO: Consider using binary search to locate the starting point. This assumes glyph
    // positions are monotonically increasing.

    // TODO: Consider cleaning this up.
    for (size_t i = 0; i < line_layout.glyphs.size(); ++i) {
        const auto& glyph = line_layout.glyphs[i];

        auto& rglyph = glyph_cache.getGlyph(glyph.font_id, glyph.glyph_id);
        int32_t left = rglyph.left;
        int32_t top = rglyph.top;
        int32_t width = rglyph.width;
        int32_t height = rglyph.height;

        // Invert glyph y-offset since we're flipping across the y-axis in OpenGL.
        top = line_height - top;

        // TODO: Refactor this whole thing... it's messy.
        int left_edge = glyph.position.x;
        int right_edge = left_edge + left + width;
        int top_edge = glyph.position.y + coords.y - metrics.descent;
        int bottom_edge = top_edge + std::max(height + top, line_height);

        if (right_edge <= min_x) continue;
        if (left_edge > max_x) return;
        if (bottom_edge <= min_y) continue;
        if (top_edge > max_y) return;

        float uv_x = rglyph.uv.x;
        float uv_y = rglyph.uv.y;
        float uv_width = rglyph.uv.z;
        float uv_height = rglyph.uv.w;

        int pos_x = coords.x + left_edge;
        int pos_y = coords.y - metrics.descent;

        if (left_edge < min_x) {
            int diff = min_x - left_edge;
            float uv_diff = static_cast<float>(diff) / Atlas::kAtlasSize;
            width -= diff;
            uv_width -= uv_diff;
            pos_x += diff;
            uv_x += uv_diff;
        }
        if (right_edge > max_x) {
            int diff = right_edge - max_x;
            float uv_diff = static_cast<float>(diff) / Atlas::kAtlasSize;
            width -= diff;
            uv_width -= uv_diff;
        }
        if (top_edge < min_y) {
            int diff = min_y - top_edge;
            int diff2 = line_height - rglyph.top;
            assert(line_height >= rglyph.top);
            int ans = std::max(diff - diff2, 0);
            float uv_diff = static_cast<float>(ans) / Atlas::kAtlasSize;
            height -= ans;
            uv_height -= uv_diff;
            pos_y += ans;
            uv_y += uv_diff;
        }
        if (bottom_edge > max_y) {
            int diff = bottom_edge - max_y;
            int diff2 = std::max(line_height - (height + top), 0);
            int ans = std::max(diff - diff2, 0);
            float uv_diff = static_cast<float>(ans) / Atlas::kAtlasSize;
            height -= ans;
            uv_height -= uv_diff;
        }

        // TODO: Remove this.
        if (width <= 0 || height <= 0) {
            continue;
        }

        // TODO: Refactor these casts.
        InstanceData instance = {
            .coords = {static_cast<float>(pos_x), static_cast<float>(pos_y)},
            .glyph = {static_cast<float>(left), static_cast<float>(top), static_cast<float>(width),
                      static_cast<float>(height)},
            .uv = {uv_x, uv_y, uv_width, uv_height},
            .color = Rgba::fromRgb(highlight_callback(glyph.index), rglyph.colored),
        };
        insertIntoBatch(rglyph.page, std::move(instance));
    }
}

void TextRenderer::insertImage(size_t image_index, const app::Point& coords, const Rgba& color) {
    const auto& glyph_cache = Renderer::instance().getGlyphCache();
    const auto& image = glyph_cache.getImage(image_index);
    InstanceData instance = {
        .coords = {static_cast<float>(coords.x), static_cast<float>(coords.y)},
        .glyph = {0, 0, static_cast<float>(image.size.width),
                  static_cast<float>(image.size.height)},
        .uv = image.uv,
        .color = color,
    };
    instance.color.a = 2;  // TODO: Use an enum.
    insertIntoBatch(image.page, std::move(instance));
}

void TextRenderer::insertColorImage(size_t image_index, const app::Point& coords) {
    const auto& glyph_cache = Renderer::instance().getGlyphCache();
    const auto& image = glyph_cache.getImage(image_index);
    InstanceData instance = {
        .coords = {static_cast<float>(coords.x), static_cast<float>(coords.y)},
        .glyph = {0, 0, static_cast<float>(image.size.width),
                  static_cast<float>(image.size.height)},
        .uv = image.uv,
    };
    instance.color.a = 3;  // TODO: Use an enum.
    insertIntoBatch(image.page, std::move(instance));
}

void TextRenderer::flush(const app::Size& screen_size) {
    const auto& glyph_cache = Renderer::instance().getGlyphCache();

    glBlendFuncSeparate(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR, GL_SRC_ALPHA, GL_ONE);

    GLuint shader_id = shader_program.id();
    glUseProgram(shader_id);
    glUniform2f(glGetUniformLocation(shader_id, "resolution"), screen_size.width,
                screen_size.height);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);

    for (size_t page = 0; page < glyph_cache.pageCount(); ++page) {
        // TODO: Refactor this ugly hack.
        while (batches.size() <= page) {
            batches.emplace_back();
            batches.back().reserve(kBatchMax);
        }

        std::vector<InstanceData>& batch = batches.at(page);
        if (batch.empty()) {
            return;
        }

        GLuint batch_tex = glyph_cache.pages().at(page).tex();
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

void TextRenderer::renderAtlasPage(
    size_t page, const app::Point& coords, int min_x, int max_x, int min_y, int max_y) {
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

    if (right_edge <= min_x) return;
    if (left_edge > max_x) return;
    if (bottom_edge <= min_y) return;
    if (top_edge > max_y) return;

    if (left_edge < min_x) {
        int diff = min_x - left_edge;
        float uv_diff = static_cast<float>(diff) / Atlas::kAtlasSize;
        width -= diff;
        uv_width -= uv_diff;
        x += diff;
        uv_x += uv_diff;
    }
    if (right_edge > max_x) {
        int diff = right_edge - max_x;
        float uv_diff = static_cast<float>(diff) / Atlas::kAtlasSize;
        width -= diff;
        uv_width -= uv_diff;
    }
    if (top_edge < min_y) {
        int diff = min_y - top_edge;
        float uv_diff = static_cast<float>(diff) / Atlas::kAtlasSize;
        height -= diff;
        uv_height -= uv_diff;
        y += diff;
        uv_y += uv_diff;
    }
    if (bottom_edge > max_y) {
        int diff = bottom_edge - max_y;
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
    insertIntoBatch(page, std::move(instance));
}

void TextRenderer::insertIntoBatch(size_t page, const InstanceData& instance) {
    // TODO: Refactor this ugly hack.
    while (batches.size() <= page) {
        batches.emplace_back();
        batches.back().reserve(kBatchMax);
    }

    std::vector<InstanceData>& batch = batches.at(page);

    batch.emplace_back(std::move(instance));
    if (batch.size() == kBatchMax) {
        fmt::println("TextRenderer error: attempted to insert into a full batch!");
    }
}

}  // namespace gui
