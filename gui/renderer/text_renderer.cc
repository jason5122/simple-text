#include "text_renderer.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

#include "opengl/gl.h"
using namespace opengl;

namespace {

const std::string kVertexShaderSource =
#include "gui/renderer/shaders/text_vert.glsl"
    ;
const std::string kFragmentShaderSource =
#include "gui/renderer/shaders/text_frag.glsl"
    ;

}

// TODO: Debug; remove this.
#include "util/profile_util.h"
#include <format>
#include <iostream>

namespace gui {

TextRenderer::TextRenderer(GlyphCache& glyph_cache)
    : glyph_cache{glyph_cache}, shader_program{kVertexShaderSource, kFragmentShaderSource} {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo_instance);
    glGenBuffers(1, &ebo);

    GLuint indices[] = {
        0, 1, 3,  // First triangle.
        1, 2, 3,  // Second triangle.
    };

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
    : glyph_cache{other.glyph_cache},
      shader_program{std::move(other.shader_program)},
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
                                    const Point& coords,
                                    int min_x,
                                    int max_x,
                                    const Rgb& color,
                                    FontType font_type) {
    const auto& font_rasterizer = font::FontRasterizer::instance();

    for (const auto& run : line_layout.runs) {
        for (const auto& glyph : run.glyphs) {
            // If we reach a glyph before the minimum x, skip it and continue.
            // If we reach a glyph *after* the maximum x, break out of the loop — we are done.
            // This assumes glyph positions are monotonically increasing.
            if (glyph.position.x + glyph.advance.x < min_x) {
                continue;
            }
            if (glyph.position.x > max_x) {
                break;
            }

            Point glyph_coords = coords;
            glyph_coords.x += glyph.position.x;

            // TODO: These changes are optimal to match Sublime Text's layout. Formalize this.
            glyph_coords.y += line_layout.ascent;

            GlyphCache::Glyph& rglyph = glyph_cache.getGlyph(
                line_layout.layout_font_id, run.font_id, glyph.glyph_id, font_rasterizer);
            const InstanceData instance{
                .coords = glyph_coords.toVec2(),
                .glyph = rglyph.glyph,
                .uv = rglyph.uv,
                .color = Rgba::fromRgb(color, rglyph.colored),
            };
            insertIntoBatch(rglyph.page, std::move(instance), font_type);
        }
    }
}

void TextRenderer::renderLineLayout(
    const font::LineLayout& line_layout,
    const Point& coords,
    int min_x,
    int max_x,
    const Rgb& color,
    FontType font_type,
    const base::SyntaxHighlighter& highlighter,
    const std::vector<base::SyntaxHighlighter::Highlight>& highlights,
    size_t line) {
    const auto& font_rasterizer = font::FontRasterizer::instance();

    auto it = highlights.begin();
    for (const auto& run : line_layout.runs) {
        for (const auto& glyph : run.glyphs) {
            TSPoint p{
                .row = static_cast<uint32_t>(line),
                .column = static_cast<uint32_t>(glyph.index),
            };

            while (it != highlights.end() && p >= (*it).end) {
                ++it;
            }

            bool is_highlight = false;
            size_t capture_index = 0;
            if (it != highlights.end() && (*it).start <= p && p < (*it).end) {
                capture_index = (*it).capture_index;
                is_highlight = true;
            }

            // If we reach a glyph before the minimum x, skip it and continue.
            // If we reach a glyph *after* the maximum x, break out of the loop — we are done.
            // This assumes glyph positions are monotonically increasing.
            if (glyph.position.x + glyph.advance.x < min_x) {
                continue;
            }
            if (glyph.position.x > max_x) {
                break;
            }

            Point glyph_coords = coords;
            glyph_coords.x += glyph.position.x;

            // TODO: These changes are optimal to match Sublime Text's layout. Formalize this.
            glyph_coords.y += line_layout.ascent;

            // TODO: Use unified Rgb struct.
            const auto& highlight_color = highlighter.getColor(capture_index);
            const Rgb rgb{.r = highlight_color.r, .g = highlight_color.g, .b = highlight_color.b};

            GlyphCache::Glyph& rglyph = glyph_cache.getGlyph(
                line_layout.layout_font_id, run.font_id, glyph.glyph_id, font_rasterizer);
            const InstanceData instance{
                .coords = glyph_coords.toVec2(),
                .glyph = rglyph.glyph,
                .uv = rglyph.uv,
                .color = Rgba::fromRgb(is_highlight ? rgb : color, rglyph.colored),
            };
            insertIntoBatch(rglyph.page, std::move(instance), font_type);
        }
    }
}

void TextRenderer::flush(const Size& screen_size, FontType font_type) {
    auto& batch_instances =
        font_type == FontType::kMain ? main_batch_instances : ui_batch_instances;

    const auto& font_rasterizer = font::FontRasterizer::instance();
    size_t font_id =
        font_type == FontType::kMain ? glyph_cache.mainFontId() : glyph_cache.uiFontId();
    const auto& metrics = font_rasterizer.getMetrics(font_id);

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);

    GLuint shader_id = shader_program.id();
    glUseProgram(shader_id);
    glUniform1f(glGetUniformLocation(shader_id, "line_height"), metrics.line_height);
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

void TextRenderer::renderAtlasPages(const Point& coords) {
    int atlas_x_offset = 0;
    for (size_t page = 0; page < glyph_cache.atlasPages().size(); ++page) {
        Point atlas_coords{
            .x = atlas_x_offset,
            .y = 0,
        };
        atlas_coords += coords;

        InstanceData instance{
            .coords = atlas_coords.toVec2(),
            .glyph = Vec4{0, 0, Atlas::kAtlasSize, Atlas::kAtlasSize},
            .uv = Vec4{0, 0, 1.0, 1.0},
            .color = Rgba{255, 255, 255, true},
        };
        insertIntoBatch(page, std::move(instance), FontType::kMain);

        atlas_x_offset += Atlas::kAtlasSize + 100;
    }
}

void TextRenderer::insertIntoBatch(size_t page, const InstanceData& instance, FontType font_type) {
    auto& batch_instances =
        font_type == FontType::kMain ? main_batch_instances : ui_batch_instances;

    // TODO: Refactor this ugly hack.
    while (batch_instances.size() <= page) {
        batch_instances.emplace_back();
        batch_instances.back().reserve(kBatchMax);
    }

    std::vector<InstanceData>& instances = batch_instances.at(page);

    instances.emplace_back(std::move(instance));
    if (instances.size() == kBatchMax) {
        std::cerr << "TextRenderer error: attempted to insert into a full batch!\n";
    }
}

}
