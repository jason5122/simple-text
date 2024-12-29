#include "text_renderer.h"

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
                                    Layer layer,
                                    const std::function<Rgb(size_t)>& highlight_callback,
                                    int min_x,
                                    int max_x,
                                    int min_y,
                                    int max_y) {
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(line_layout.layout_font_id);

    // TODO: Consider using binary search to locate the starting point. This assumes glyph
    // positions are monotonically increasing.

    // If we reach a glyph before the minimum x, skip it and continue.
    // If we reach a glyph *after* the maximum x, break out of the loop — we are done.
    // This assumes glyph positions are monotonically increasing.
    for (size_t i = 0; i < line_layout.glyphs.size(); ++i) {
        const auto& glyph = line_layout.glyphs[i];

        auto& rglyph = glyph_cache.getGlyph(glyph.font_id, glyph.glyph_id);
        int32_t left = rglyph.left;
        int32_t top = rglyph.top;
        int32_t width = rglyph.width;
        int32_t height = rglyph.height;

        // Invert glyph y-offset since we're flipping across the y-axis in OpenGL.
        top = metrics.line_height - top;

        // TODO: Refactor this whole thing... it's messy.
        int left_edge = glyph.position.x;
        int right_edge = left_edge + left + width;

        if (right_edge < min_x) {
            continue;
        }
        if (left_edge >= max_x) {
            return;
        }

        auto pos = coords;
        pos.x += left_edge;
        pos.y -= metrics.descent;

        float uv_left = rglyph.uv.x;
        float uv_bot = rglyph.uv.y;
        float uv_width = rglyph.uv.z;
        float uv_height = rglyph.uv.w;

        if (right_edge > max_x) {
            int diff = std::max(right_edge - max_x, 0);
            float uv_diff = static_cast<float>(diff) / Atlas::kAtlasSize;
            // if (diff > width) {
            //     fmt::println("WHAT #1 {} {}", diff, width);
            // }
            width -= diff;
            uv_width -= uv_diff;
        }
        if (left_edge < min_x) {
            int diff = std::max(min_x - glyph.position.x, 0);
            float uv_diff = static_cast<float>(diff) / Atlas::kAtlasSize;
            // if (diff > width) {
            //     fmt::println("WHAT #2 {} {}", diff, width);
            // }
            width -= diff;
            uv_width -= uv_diff;
            left += diff;
            uv_left += uv_diff;
        }

        InstanceData instance = {
            .coords = {static_cast<float>(pos.x), static_cast<float>(pos.y)},
            // TODO: Refactor these casts.
            .glyph = {static_cast<float>(left), static_cast<float>(top), static_cast<float>(width),
                      static_cast<float>(height)},
            .uv = {uv_left, uv_bot, uv_width, uv_height},
            .color = Rgba::fromRgb(highlight_callback(glyph.index), rglyph.colored),
        };
        insertIntoBatch(rglyph.page, std::move(instance), layer);
    }
}

void TextRenderer::flush(const app::Size& screen_size, Layer layer) {
    auto& batch_instances = layer == Layer::kOne ? layer_one_instances : layer_two_instances;

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);

    GLuint shader_id = shader_program.id();
    glUseProgram(shader_id);
    glUniform2f(glGetUniformLocation(shader_id, "resolution"), screen_size.width,
                screen_size.height);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    for (size_t page = 0; page < glyph_cache.atlasPages().size(); ++page) {
        // TODO: Refactor this ugly hack.
        while (batch_instances.size() <= page) {
            batch_instances.emplace_back();
            batch_instances.back().reserve(kBatchMax);
        }

        GLuint batch_tex = glyph_cache.atlasPages().at(page).tex();
        std::vector<InstanceData>& instances = batch_instances.at(page);

        if (instances.empty()) {
            return;
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(),
                        instances.data());

        glBindTexture(GL_TEXTURE_2D, batch_tex);

        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

        instances.clear();
    }

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextRenderer::renderAtlasPages(const app::Point& coords) {
    int atlas_x_offset = 0;
    for (size_t page = 0; page < glyph_cache.atlasPages().size(); ++page) {
        app::Point atlas_coords{
            .x = atlas_x_offset,
            .y = 0,
        };
        atlas_coords += coords;

        const Vec2 coords_vec = Vec2{
            .x = static_cast<float>(atlas_coords.x),
            .y = static_cast<float>(atlas_coords.y),
        };
        InstanceData instance{
            .coords = coords_vec,
            .glyph = Vec4{0, 0, Atlas::kAtlasSize, Atlas::kAtlasSize},
            .uv = Vec4{0, 0, 1.0, 1.0},
            .color = Rgba{255, 255, 255, true},
        };
        insertIntoBatch(page, std::move(instance), Layer::kOne);

        atlas_x_offset += Atlas::kAtlasSize + 100;
    }
}

void TextRenderer::insertIntoBatch(size_t page, const InstanceData& instance, Layer layer) {
    auto& batch_instances = layer == Layer::kOne ? layer_one_instances : layer_two_instances;

    // TODO: Refactor this ugly hack.
    while (batch_instances.size() <= page) {
        batch_instances.emplace_back();
        batch_instances.back().reserve(kBatchMax);
    }

    std::vector<InstanceData>& instances = batch_instances.at(page);

    instances.emplace_back(std::move(instance));
    if (instances.size() == kBatchMax) {
        fmt::println("TextRenderer error: attempted to insert into a full batch!");
    }
}

}  // namespace gui
