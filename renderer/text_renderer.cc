#include "base/filesystem/file_reader.h"
#include "base/rgb.h"
#include "renderer/opengl_error_util.h"
#include "renderer/selection_renderer.h"
#include "text_renderer.h"
#include "util/profile_util.h"
#include <cmath>
#include <cstdint>
#include <memory>

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

#include "build/buildflag.h"

namespace renderer {
TextRenderer::TextRenderer(GlyphCache& main_glyph_cache, GlyphCache& ui_glyph_cache)
    : main_glyph_cache(main_glyph_cache), ui_glyph_cache(ui_glyph_cache) {}

TextRenderer::~TextRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo_instance);
    glDeleteBuffers(1, &ebo);
}

void TextRenderer::setup() {
    std::string vert_source =
#include "shaders/text_vert.glsl"
        ;
    std::string frag_source =
#include "shaders/text_frag.glsl"
        ;

    shader_program.link(vert_source, frag_source);

    glUseProgram(shader_program.id);
    glUniform1f(glGetUniformLocation(shader_program.id, "line_height"),
                main_glyph_cache.lineHeight());

    GLuint indices[] = {
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData) * kBatchMax, nullptr, GL_STATIC_DRAW);

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

void TextRenderer::renderText(Size& size, Point& scroll, Buffer& buffer,
                              SyntaxHighlighter& highlighter, Point& editor_offset,
                              CaretInfo& start_caret, CaretInfo& end_caret, int& longest_line_x,
                              config::ColorScheme& color_scheme, int line_number_offset) {
    glUseProgram(shader_program.id);
    glUniform2f(glGetUniformLocation(shader_program.id, "resolution"), size.width, size.height);
    glUniform2f(glGetUniformLocation(shader_program.id, "scroll_offset"), scroll.x, scroll.y);
    glUniform2f(glGetUniformLocation(shader_program.id, "editor_offset"), editor_offset.x,
                editor_offset.y);
    glUniform1f(glGetUniformLocation(shader_program.id, "line_number_offset"), line_number_offset);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    size_t start_line = std::min(static_cast<size_t>(scroll.y / main_glyph_cache.lineHeight()),
                                 buffer.lineCount());
    size_t visible_lines =
        std::ceil((size.height - ui_glyph_cache.lineHeight()) / main_glyph_cache.lineHeight());
    size_t end_line = std::min(start_line + visible_lines, buffer.lineCount());

    {
        PROFILE_BLOCK("Tree-sitter highlight");
        highlighter.getHighlights({static_cast<uint32_t>(start_line), 0},
                                  {static_cast<uint32_t>(end_line), 0});
    }

    size_t byte_offset = buffer.byteOfLine(start_line);

    std::vector<InstanceData> instances;
    instances.reserve(kBatchMax);

    auto render_batch = [this, &instances]() {
        glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(),
                        &instances[0]);

        main_glyph_cache.bindTexture();

        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

        instances.clear();
    };

    {
        PROFILE_BLOCK("layout text");

        size_t selection_start = start_caret.byte;
        size_t selection_end = end_caret.byte;
        if (selection_start > selection_end) {
            std::swap(selection_start, selection_end);
        }

        for (size_t line_index = start_line; line_index < end_line; line_index++) {
            size_t ret;
            int total_advance = 0;

            // Draw line number.
            std::string line_number_str = std::to_string(line_index + 1);
            std::reverse(line_number_str.begin(), line_number_str.end());

            for (size_t offset = 0; offset < line_number_str.size(); offset += ret) {
                ret = grapheme_next_character_break_utf8(&line_number_str[0] + offset, SIZE_MAX);
                std::string_view key = std::string_view(line_number_str).substr(offset, ret);
                AtlasGlyph& glyph = main_glyph_cache.getGlyph(key);

                Vec2 coords{
                    .x = static_cast<float>(-total_advance - line_number_offset / 2),
                    .y = static_cast<float>(line_index * main_glyph_cache.lineHeight()),
                };
                instances.emplace_back(InstanceData{
                    .coords = coords,
                    .glyph = glyph.glyph,
                    .uv = glyph.uv,
                    .color = Rgba::fromRgb(Rgb{150, 150, 150}, glyph.colored),
                });

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

                std::string_view key = std::string_view(line_str).substr(offset, ret);

                // TODO: Preserve the width of the space character when substituting.
                //       Otherwise, the line width changes when using proportional fonts.
                Rgb text_color = highlighter.getColor(byte_offset, color_scheme);
                if (key == " " && selection_start <= byte_offset && byte_offset < selection_end) {
                    key = "·";
                    text_color = Rgb{182, 182, 182};
                }
                AtlasGlyph& glyph = main_glyph_cache.getGlyph(key);

                Vec2 coords{
                    .x = static_cast<float>(total_advance),
                    .y = static_cast<float>(line_index * main_glyph_cache.lineHeight()),
                };
                instances.emplace_back(InstanceData{
                    .coords = coords,
                    .glyph = glyph.glyph,
                    .uv = glyph.uv,
                    .color = Rgba::fromRgb(text_color, glyph.colored),
                });
                if (instances.size() == kBatchMax) {
                    render_batch();
                }

                total_advance += glyph.advance;
            }

            byte_offset++;
            longest_line_x = std::max(total_advance, longest_line_x);
        }
    }

    // instances.emplace_back(InstanceData{
    //     .coords =
    //         Vec2{
    //             .x = static_cast<float>(size.width - Atlas::kAtlasSize - 400 + scroll.x),
    //             .y = static_cast<float>(10 * main_glyph_cache.lineHeight() + scroll.y),
    //         },
    //     .glyph = Vec4{0, 0, Atlas::kAtlasSize, Atlas::kAtlasSize},
    //     .uv = Vec4{0, 0, 1.0, 1.0},
    //     .color = Rgba::fromRgb(color_scheme.foreground, false),
    // });

    render_batch();

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind.
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glCheckError();
}

std::vector<SelectionRenderer::Selection>
TextRenderer::getSelections(Buffer& buffer, CaretInfo& start_caret, CaretInfo& end_caret) {
    std::vector<SelectionRenderer::Selection> selections;

    size_t start_byte = start_caret.byte, end_byte = end_caret.byte;
    size_t start_line = start_caret.line, end_line = end_caret.line;

    if (start_byte == end_byte) {
        return selections;
    }
    if (start_byte > end_byte) {
        std::swap(start_byte, end_byte);
        std::swap(start_line, end_line);
    }

    size_t byte_offset = buffer.byteOfLine(start_line);
    for (size_t line_index = start_line; line_index <= end_line; line_index++) {

        std::string line_str = buffer.getLineContent(line_index);

        int total_advance = 0;
        int start = 0;
        int end = 0;

        size_t ret;
        for (size_t offset = 0; offset < line_str.size(); offset += ret, byte_offset += ret) {
            ret = grapheme_next_character_break_utf8(&line_str[0] + offset, SIZE_MAX);
            std::string_view key = std::string_view(line_str).substr(offset, ret);
            AtlasGlyph& glyph = main_glyph_cache.getGlyph(key);

            if (byte_offset == start_byte) {
                start = total_advance;
            }
            if (byte_offset == end_byte) {
                end = total_advance;
            }

            total_advance += glyph.advance;
        }
        if (byte_offset == start_byte) {
            start = total_advance;
        }
        if (byte_offset == end_byte) {
            end = total_advance;
        }
        byte_offset++;

        if (line_index != end_line) {
            std::string_view key = " ";
            AtlasGlyph& space_glyph = main_glyph_cache.getGlyph(key);
            end = total_advance + space_glyph.advance;
        }

        if (start != end) {
            selections.emplace_back(SelectionRenderer::Selection{
                .line = static_cast<int>(line_index),
                .start = start,
                .end = end,
            });
        }
    }

    return selections;
}

std::vector<int>
TextRenderer::getTabTitleWidths(Buffer& buffer,
                                std::vector<std::unique_ptr<EditorTab>>& editor_tabs) {
    std::vector<int> tab_title_widths;

    auto add_width = [&](std::string_view str) {
        size_t ret;
        int total_advance = 0;
        for (size_t offset = 0; offset < str.size(); offset += ret) {
            ret = grapheme_next_character_break_utf8(&str[0] + offset, SIZE_MAX);
            std::string_view key = str.substr(offset, ret);
            AtlasGlyph& glyph = ui_glyph_cache.getGlyph(key);

            total_advance += glyph.advance;
        }

        // TODO: Replace this magic number with the width of the close button.
        total_advance += 32;

        tab_title_widths.emplace_back(total_advance);
    };

    for (const auto& editor_tab : editor_tabs) {
        fs::path&& tab_name = editor_tab->file_path.filename();
        if (editor_tab->file_path.empty()) {
            tab_name = "untitled";
        }
        std::string tab_name_str = tab_name.string();
        add_width(tab_name_str);
    }

    return tab_title_widths;
}

void TextRenderer::renderUiText(Size& size, CaretInfo& end_caret,
                                config::ColorScheme& color_scheme, Point& editor_offset,
                                std::vector<std::unique_ptr<EditorTab>>& editor_tabs,
                                std::vector<int>& tab_title_x_coords) {
    glUseProgram(shader_program.id);
    glUniform2f(glGetUniformLocation(shader_program.id, "resolution"), size.width, size.height);
    glUniform2f(glGetUniformLocation(shader_program.id, "scroll_offset"), 0, 0);
    glUniform2f(glGetUniformLocation(shader_program.id, "editor_offset"), 0, 0);
    glUniform1f(glGetUniformLocation(shader_program.id, "line_number_offset"), 0);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    std::vector<InstanceData> instances;

    auto create_instances = [&](std::string_view str, int x_offset, int y_offset) {
        size_t ret;
        int total_advance = 0;
        for (size_t offset = 0; offset < str.size(); offset += ret) {
            ret = grapheme_next_character_break_utf8(&str[0] + offset, SIZE_MAX);
            std::string_view key = str.substr(offset, ret);
            AtlasGlyph& glyph = ui_glyph_cache.getGlyph(key);

            instances.emplace_back(InstanceData{
                .coords =
                    Vec2{
                        .x = static_cast<float>(total_advance + x_offset),
                        .y = static_cast<float>(y_offset),
                    },
                .glyph = glyph.glyph,
                .uv = glyph.uv,
                .color = Rgba::fromRgb(color_scheme.foreground, glyph.colored),
            });

            total_advance += glyph.advance;
        }
    };

    int status_text_offset = 25;  // TODO: Convert this magic number to actual code.
    std::string line_str = std::format("Line {}, Column {}", end_caret.line, end_caret.column);

    int y_bottom_of_screen = size.height - main_glyph_cache.lineHeight();
    int y_top_of_screen =
        editor_offset.y / 2 + ui_glyph_cache.lineHeight() / 2 - main_glyph_cache.lineHeight();

    // TODO: Replace magic numbers and strings.
    std::string json = "JSON";
    std::string indentation = "Spaces: 4";

    create_instances(line_str, status_text_offset, y_bottom_of_screen);
    create_instances(json, size.width - 150, y_bottom_of_screen);
    create_instances(indentation, size.width - 350, y_bottom_of_screen);

    for (size_t i = 0; i < editor_tabs.size(); i++) {
        fs::path&& tab_name = editor_tabs.at(i)->file_path.filename();
        if (editor_tabs.at(i)->file_path.empty()) {
            tab_name = "untitled";
        }
        std::string tab_name_str = tab_name.string();
        create_instances(tab_name_str, editor_offset.x + tab_title_x_coords.at(i),
                         y_top_of_screen);
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);

    ui_glyph_cache.bindTexture();

    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glCheckError();
}
}
