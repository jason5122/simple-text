#include "base/rgb.h"
#include "opengl/functions_gl_enums.h"
#include "text_renderer.h"
#include "util/profile_util.h"
#include <cmath>
#include <cstdint>

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

#include "build/buildflag.h"

namespace {
const std::string kVertexShaderSource =
#include "renderer/shaders/text_vert.glsl"
    ;
const std::string kFragmentShaderSource =
#include "renderer/shaders/text_frag.glsl"
    ;
}

namespace renderer {

TextRenderer::TextRenderer(std::shared_ptr<opengl::FunctionsGL> shared_gl,
                           GlyphCache& main_glyph_cache,
                           GlyphCache& ui_glyph_cache)
    : gl{std::move(shared_gl)},
      shader_program{gl, kVertexShaderSource, kFragmentShaderSource},
      main_glyph_cache{main_glyph_cache},
      ui_glyph_cache{ui_glyph_cache} {
    gl->genVertexArrays(1, &vao);
    gl->genBuffers(1, &vbo_instance);
    gl->genBuffers(1, &ebo);

    GLuint indices[] = {
        0, 1, 3,  // First triangle.
        1, 2, 3,  // Second triangle.
    };

    gl->bindVertexArray(vao);

    gl->bindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    gl->bufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    gl->bindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    gl->bufferData(GL_ARRAY_BUFFER, sizeof(InstanceData) * kBatchMax, nullptr, GL_STATIC_DRAW);

    GLuint index = 0;

    gl->enableVertexAttribArray(index);
    gl->vertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                            (void*)offsetof(InstanceData, coords));
    gl->vertexAttribDivisor(index++, 1);

    gl->enableVertexAttribArray(index);
    gl->vertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                            (void*)offsetof(InstanceData, glyph));
    gl->vertexAttribDivisor(index++, 1);

    gl->enableVertexAttribArray(index);
    gl->vertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                            (void*)offsetof(InstanceData, uv));
    gl->vertexAttribDivisor(index++, 1);

    gl->enableVertexAttribArray(index);
    gl->vertexAttribPointer(index, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(InstanceData),
                            (void*)offsetof(InstanceData, color));
    gl->vertexAttribDivisor(index++, 1);

    // Unbind.
    gl->bindVertexArray(0);
    gl->bindBuffer(GL_ARRAY_BUFFER, 0);
    gl->bindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

TextRenderer::~TextRenderer() {
    gl->deleteVertexArrays(1, &vao);
    gl->deleteBuffers(1, &vbo_instance);
    gl->deleteBuffers(1, &ebo);
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

    GLuint shader_id = shader_program.id();
    gl->useProgram(shader_id);
    gl->uniform1f(gl->getUniformLocation(shader_id, "line_height"), main_glyph_cache.lineHeight());
    gl->uniform2f(gl->getUniformLocation(shader_id, "resolution"), size.width, size.height);
    gl->uniform2f(gl->getUniformLocation(shader_id, "scroll_offset"), scroll.x, scroll.y);
    gl->uniform2f(gl->getUniformLocation(shader_id, "editor_offset"), editor_offset.x,
                  editor_offset.y);
    gl->uniform1f(gl->getUniformLocation(shader_id, "line_number_offset"), line_number_offset);

    gl->activeTexture(GL_TEXTURE0);
    gl->bindVertexArray(vao);

    int batch_count = 0;
    auto render_batch = [this, &batch_count](size_t page) {
        while (batch_instances.size() <= page) {
            batch_instances.emplace_back();
            batch_instances.back().reserve(kBatchMax);
        }

        GLuint batch_tex = main_glyph_cache.atlas_pages.at(page).tex();
        std::vector<InstanceData>& instances = batch_instances.at(page);

        if (instances.empty()) {
            return;
        }

        gl->bindBuffer(GL_ARRAY_BUFFER, vbo_instance);
        gl->bufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(),
                          &instances[0]);

        gl->bindTexture(GL_TEXTURE_2D, batch_tex);

        gl->drawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

        instances.clear();
        batch_count++;
    };

    auto insert_into_batch = [this, &render_batch](size_t page, const InstanceData& instance) {
        while (batch_instances.size() <= page) {
            batch_instances.emplace_back();
            batch_instances.back().reserve(kBatchMax);
        }

        std::vector<InstanceData>& instances = batch_instances.at(page);

        instances.emplace_back(std::move(instance));
        if (instances.size() == kBatchMax) {
            render_batch(page);
        }
    };

    {
        PROFILE_BLOCK("layout text");

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
                    .x = static_cast<float>(-total_advance - line_number_offset / 2),
                    .y = static_cast<float>(line_index * main_glyph_cache.lineHeight()),
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
                    .x = static_cast<float>(total_advance),
                    .y = static_cast<float>(line_index * main_glyph_cache.lineHeight()),
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

        render_batch(page);

        atlas_x_offset += Atlas::kAtlasSize + 100;
    }

    // Unbind.
    gl->bindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind.
    gl->bindVertexArray(0);
    gl->bindTexture(GL_TEXTURE_2D, 0);
}

}
