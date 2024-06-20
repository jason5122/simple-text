#include "base/rgb.h"
#include "text_renderer.h"
#include <cmath>
#include <cstdint>

#include "opengl/functions_gl_enums.h"
#include "opengl/gl.h"
using namespace opengl;

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

namespace {
const std::string kVertexShaderSource =
#include "renderer/shaders/text_vert.glsl"
    ;
const std::string kFragmentShaderSource =
#include "renderer/shaders/text_frag.glsl"
    ;
}

// TODO: Debug; remove this.
#include <format>
#include <iostream>

namespace renderer {

TextRenderer::TextRenderer(std::shared_ptr<opengl::FunctionsGL> shared_gl,
                           GlyphCache& main_glyph_cache,
                           GlyphCache& ui_glyph_cache)
    : gl{std::move(shared_gl)},
      shader_program{gl, kVertexShaderSource, kFragmentShaderSource},
      main_glyph_cache{main_glyph_cache},
      ui_glyph_cache{ui_glyph_cache} {
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
    : vao{other.vao},
      vbo_instance{other.vbo_instance},
      ebo{other.ebo},
      gl{other.gl},
      shader_program{std::move(other.shader_program)},
      main_glyph_cache{other.main_glyph_cache},
      ui_glyph_cache{other.ui_glyph_cache} {
    other.vao = 0;
    other.vbo_instance = 0;
    other.ebo = 0;
}

TextRenderer& TextRenderer::operator=(TextRenderer&& other) {
    if (&other != this) {
        vao = other.vao;
        vbo_instance = other.vbo_instance;
        ebo = other.ebo;
        shader_program = std::move(other.shader_program);
        other.vao = 0;
        other.vbo_instance = 0;
        other.ebo = 0;
    }
    return *this;
}

void TextRenderer::renderText(const Size& size,
                              const Point& scroll,
                              const base::Buffer& buffer,
                              const Point& editor_offset,
                              const CaretInfo& start_caret,
                              const CaretInfo& end_caret,
                              int& longest_line_x,
                              Point& end_caret_pos) {
    // TODO: Clean this up.
    int line_number_offset = 100;

    auto insert_into_batch = [this](size_t page, const InstanceData& instance) {
        while (batch_instances.size() <= page) {
            batch_instances.emplace_back();
            batch_instances.back().reserve(kBatchMax);
        }

        std::vector<InstanceData>& instances = batch_instances.at(page);

        instances.emplace_back(std::move(instance));
        if (instances.size() == kBatchMax) {
            std::cerr << "TextRenderer error: attempted to insert into a full batch!\n";
        }
    };

    size_t selection_start = start_caret.byte;
    size_t selection_end = end_caret.byte;
    if (selection_start > selection_end) {
        std::swap(selection_start, selection_end);
    }

    size_t start_line = std::min(static_cast<size_t>(scroll.y / main_glyph_cache.lineHeight()),
                                 buffer.lineCount());
    size_t visible_lines =
        std::ceil((size.height - ui_glyph_cache.lineHeight()) / main_glyph_cache.lineHeight());
    size_t end_line = std::min(start_line + visible_lines, buffer.lineCount());

    start_line = 0;
    end_line = buffer.lineCount();

    size_t byte_offset = buffer.byteOfLine(start_line);

    for (size_t line_index = start_line; line_index < end_line; line_index++) {
        size_t ret;
        int total_advance = 0;

        // Draw line number.
        std::string line_number_str = std::to_string(line_index + 1);
        std::reverse(line_number_str.begin(), line_number_str.end());

        for (size_t offset = 0; offset < line_number_str.size(); offset += ret) {
            ret = grapheme_next_character_break_utf8(&line_number_str[0] + offset, SIZE_MAX);
            std::string_view key = std::string_view(line_number_str).substr(offset, ret);
            GlyphCache::Glyph& glyph = main_glyph_cache.getGlyph(key);

            Vec2 coords{
                .x = static_cast<float>(-total_advance - line_number_offset / 2) +
                     editor_offset.x + line_number_offset - scroll.x,
                .y = static_cast<float>(line_index * main_glyph_cache.lineHeight()) +
                     editor_offset.y - scroll.y,
            };

            InstanceData instance{
                .coords = coords,
                .glyph = glyph.glyph,
                .uv = glyph.uv,
                .color = Rgba::fromRgb(base::Rgb{150, 150, 150}, glyph.colored),
            };
            insert_into_batch(glyph.page, std::move(instance));

            total_advance += glyph.advance;
        }

        total_advance = 0;
        std::string line_str = buffer.getLineContent(line_index);

        for (size_t offset = 0; offset < line_str.size(); offset += ret, byte_offset += ret) {
            ret = grapheme_next_character_break_utf8(&line_str[0] + offset, SIZE_MAX);

            if (total_advance > size.width) {
                byte_offset += line_str.size() - offset;
                break;
            }

            if (byte_offset == end_caret.byte) {
                end_caret_pos = {
                    .x = total_advance,
                    .y = static_cast<int>(line_index) * main_glyph_cache.lineHeight(),
                };
            }

            std::string_view key = std::string_view(line_str).substr(offset, ret);

            // TODO: Preserve the width of the space character when substituting.
            //       Otherwise, the line width changes when using proportional fonts.
            base::Rgb text_color{51, 51, 51};
            if (key == " " && selection_start <= byte_offset && byte_offset < selection_end) {
                key = "·";
                text_color = base::Rgb{182, 182, 182};
            }
            GlyphCache::Glyph& glyph = main_glyph_cache.getGlyph(key);

            Vec2 coords{
                .x = static_cast<float>(total_advance) + editor_offset.x + line_number_offset -
                     scroll.x,
                .y = static_cast<float>(line_index * main_glyph_cache.lineHeight()) +
                     editor_offset.y - scroll.y,
            };

            InstanceData instance{
                .coords = coords,
                .glyph = glyph.glyph,
                .uv = glyph.uv,
                .color = Rgba::fromRgb(text_color, glyph.colored),
            };
            insert_into_batch(glyph.page, std::move(instance));

            total_advance += glyph.advance;
        }

        if (byte_offset == end_caret.byte) {
            end_caret_pos = {
                .x = total_advance,
                .y = static_cast<int>(line_index) * main_glyph_cache.lineHeight(),
            };
        }
        byte_offset++;

        longest_line_x = std::max(total_advance, longest_line_x);
    }

    int atlas_x_offset = 0;
    for (size_t page = 0; page < main_glyph_cache.atlas_pages.size(); page++) {
        // TODO: Incorporate this into the build system.
        bool debug_atlas = false;
        if (debug_atlas) {
            InstanceData instance{
                .coords =
                    Vec2{
                        .x = static_cast<float>(scroll.x + atlas_x_offset),
                        .y = static_cast<float>(size.height - Atlas::kAtlasSize - 200 + scroll.y),
                    },
                .glyph = Vec4{0, 0, Atlas::kAtlasSize, Atlas::kAtlasSize},
                .uv = Vec4{0, 0, 1.0, 1.0},
                .color = Rgba{255, 0, 0, 0},
            };
            insert_into_batch(page, std::move(instance));
        }

        atlas_x_offset += Atlas::kAtlasSize + 100;
    }
}

void TextRenderer::flush(const Size& size) {
    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);

    GLuint shader_id = shader_program.id();
    glUseProgram(shader_id);
    glUniform1f(glGetUniformLocation(shader_id, "line_height"), main_glyph_cache.lineHeight());
    glUniform2f(glGetUniformLocation(shader_id, "resolution"), size.width, size.height);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    for (size_t page = 0; page < main_glyph_cache.atlas_pages.size(); page++) {
        while (batch_instances.size() <= page) {
            batch_instances.emplace_back();
            batch_instances.back().reserve(kBatchMax);
        }

        GLuint batch_tex = main_glyph_cache.atlas_pages.at(page).tex();
        std::vector<InstanceData>& instances = batch_instances.at(page);

        if (instances.empty()) {
            return;
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(),
                        &instances[0]);

        glBindTexture(GL_TEXTURE_2D, batch_tex);

        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

        instances.clear();
    }

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

}
