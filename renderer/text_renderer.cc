#include "base/filesystem/file_reader.h"
#include "base/rgb.h"
#include "font/rasterizer.h"
#include "renderer/opengl_error_util.h"
#include "renderer/selection_renderer.h"
#include "text_renderer.h"
#include "util/profile_util.h"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

#include "build/buildflag.h"

namespace renderer {
TextRenderer::TextRenderer() {}

TextRenderer::~TextRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo_instance);
    glDeleteBuffers(1, &ebo);
}

void TextRenderer::setup(FontRasterizer& font_rasterizer) {
    std::string vert_source =
#include "shaders/text_vert.glsl"
        ;
    std::string frag_source =
#include "shaders/text_frag.glsl"
        ;

    shader_program.link(vert_source, frag_source);

    // TODO: Replace this with a more permanent solution.
    glyph_caches.emplace_back();
    glyph_caches.emplace_back();
    ascii_caches.emplace_back();
    ascii_caches.emplace_back();

    atlas.setup();

    glUseProgram(shader_program.id);
    glUniform1f(glGetUniformLocation(shader_program.id, "line_height"),
                font_rasterizer.line_height);

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

TextRenderer::AtlasGlyph& TextRenderer::getAtlasGlyph(std::string_view key,
                                                      FontRasterizer& font_rasterizer) {
    // If the input is ASCII (a very common occurrence), we can skip the overhead of a hash map.
    if (key.length() == 1 && 0x20 <= key[0] && key[0] <= 0x7e) {
        ascii_cache_type& ascii_cache = ascii_caches.at(font_rasterizer.id);

        size_t i = key[0] - 0x20;
        if (ascii_cache.at(i) == std::nullopt) {
            ascii_cache.at(i) = createAtlasGlyph(key, font_rasterizer);
        }
        return ascii_cache.at(i).value();
    }

    cache_type& glyph_cache = glyph_caches.at(font_rasterizer.id);

    auto it = glyph_cache.find(key);
    if (it == glyph_cache.end()) {
        it = glyph_cache.emplace(key, createAtlasGlyph(key, font_rasterizer)).first;
    }
    return it->second;
}

float TextRenderer::getGlyphAdvance(std::string_view utf8_str, FontRasterizer& font_rasterizer) {
    AtlasGlyph& glyph = getAtlasGlyph(utf8_str, font_rasterizer);
    return std::round(glyph.advance);
}

// TODO: Rewrite this so this operates on an already shaped line.
//       We should remove any glyph cache/font rasterization from this method.
std::pair<float, size_t> TextRenderer::closestBoundaryForX(std::string_view line_str, float x,
                                                           FontRasterizer& font_rasterizer) {
    size_t offset;
    size_t ret;
    float total_advance = 0;
    for (offset = 0; offset < line_str.size(); offset += ret) {
        ret = grapheme_next_character_break_utf8(&line_str[0] + offset, SIZE_MAX);
        std::string_view key = line_str.substr(offset, ret);
        AtlasGlyph& glyph = getAtlasGlyph(key, font_rasterizer);

        float glyph_center = total_advance + glyph.advance / 2;
        if (glyph_center >= x) {
            return {total_advance, offset};
        }

        total_advance += std::round(glyph.advance);
    }
    return {total_advance, offset};
}

void TextRenderer::renderText(Size& size, Point& scroll, Buffer& buffer,
                              SyntaxHighlighter& highlighter, Point& editor_offset,
                              FontRasterizer& font_rasterizer, float status_bar_height,
                              CaretInfo& start_caret, CaretInfo& end_caret, float& longest_line_x,
                              config::ColorScheme& color_scheme, float line_number_offset) {
    glUseProgram(shader_program.id);
    glUniform2f(glGetUniformLocation(shader_program.id, "resolution"), size.width, size.height);
    glUniform2f(glGetUniformLocation(shader_program.id, "scroll_offset"), scroll.x, scroll.y);
    glUniform2f(glGetUniformLocation(shader_program.id, "editor_offset"), editor_offset.x,
                editor_offset.y);
    glUniform1f(glGetUniformLocation(shader_program.id, "line_number_offset"), line_number_offset);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    size_t start_line =
        std::min(static_cast<size_t>(scroll.y / font_rasterizer.line_height), buffer.lineCount());
    size_t visible_lines =
        std::ceil((size.height - status_bar_height) / font_rasterizer.line_height);
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

        glBindTexture(GL_TEXTURE_2D, atlas.tex_id);

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
            float total_advance = 0;

            // Draw line number.
            std::string line_number_str = std::to_string(line_index + 1);
            std::reverse(line_number_str.begin(), line_number_str.end());

            for (size_t offset = 0; offset < line_number_str.size(); offset += ret) {
                ret = grapheme_next_character_break_utf8(&line_number_str[0] + offset, SIZE_MAX);
                std::string_view key = std::string_view(line_number_str).substr(offset, ret);
                AtlasGlyph& glyph = getAtlasGlyph(key, font_rasterizer);

                Vec2 coords{-total_advance - line_number_offset / 2,
                            line_index * font_rasterizer.line_height};
                instances.emplace_back(InstanceData{
                    .coords = coords,
                    .glyph = glyph.glyph,
                    .uv = glyph.uv,
                    .color = Rgba::fromRgb(Rgb{150, 150, 150}, glyph.colored),
                });

                total_advance += std::round(glyph.advance);
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
                AtlasGlyph& glyph = getAtlasGlyph(key, font_rasterizer);

                Vec2 coords{total_advance, line_index * font_rasterizer.line_height};
                instances.emplace_back(InstanceData{
                    .coords = coords,
                    .glyph = glyph.glyph,
                    .uv = glyph.uv,
                    .color = Rgba::fromRgb(text_color, glyph.colored),
                });
                if (instances.size() == kBatchMax) {
                    render_batch();
                }

                total_advance += std::round(glyph.advance);
            }

            byte_offset++;
            longest_line_x = std::max(total_advance, longest_line_x);
        }
    }

    instances.emplace_back(InstanceData{
        .coords = Vec2{size.width - Atlas::kAtlasSize - 400 + scroll.x,
                       10 * font_rasterizer.line_height + scroll.y},
        .glyph = Vec4{0, 0, Atlas::kAtlasSize, Atlas::kAtlasSize},
        .uv = Vec4{0, 0, 1.0, 1.0},
        .color = Rgba::fromRgb(color_scheme.foreground, false),
    });

    render_batch();

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind.
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glCheckError();
}

std::vector<SelectionRenderer::Selection>
TextRenderer::getSelections(Buffer& buffer, FontRasterizer& font_rasterizer,
                            CaretInfo& start_caret, CaretInfo& end_caret) {
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

        float total_advance = 0;
        int start = 0;
        int end = 0;

        size_t ret;
        for (size_t offset = 0; offset < line_str.size(); offset += ret, byte_offset += ret) {
            ret = grapheme_next_character_break_utf8(&line_str[0] + offset, SIZE_MAX);
            std::string_view key = std::string_view(line_str).substr(offset, ret);
            AtlasGlyph& glyph = getAtlasGlyph(key, font_rasterizer);

            if (byte_offset == start_byte) {
                start = total_advance;
            }
            if (byte_offset == end_byte) {
                end = total_advance;
            }

            total_advance += std::round(glyph.advance);
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
            AtlasGlyph& space_glyph = getAtlasGlyph(key, font_rasterizer);
            end = static_cast<int>(total_advance) + std::round(space_glyph.advance);
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
TextRenderer::getTabTitleWidths(Buffer& buffer, FontRasterizer& ui_font_rasterizer,
                                std::vector<std::unique_ptr<EditorTab>>& editor_tabs) {
    std::vector<int> tab_title_widths;

    auto add_width = [&](std::string_view str) {
        size_t ret;
        float total_advance = 0;
        for (size_t offset = 0; offset < str.size(); offset += ret) {
            ret = grapheme_next_character_break_utf8(&str[0] + offset, SIZE_MAX);
            std::string_view key = str.substr(offset, ret);
            AtlasGlyph& glyph = getAtlasGlyph(key, ui_font_rasterizer);

            total_advance += std::round(glyph.advance);
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

void TextRenderer::renderUiText(Size& size, FontRasterizer& main_font_rasterizer,
                                FontRasterizer& ui_font_rasterizer, CaretInfo& end_caret,
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
        float total_advance = 0;
        for (size_t offset = 0; offset < str.size(); offset += ret) {
            ret = grapheme_next_character_break_utf8(&str[0] + offset, SIZE_MAX);
            std::string_view key = str.substr(offset, ret);
            AtlasGlyph& glyph = getAtlasGlyph(key, ui_font_rasterizer);

            instances.emplace_back(InstanceData{
                .coords = Vec2{total_advance + x_offset, static_cast<float>(y_offset)},
                .glyph = glyph.glyph,
                .uv = glyph.uv,
                .color = Rgba::fromRgb(color_scheme.foreground, glyph.colored),
            });

            total_advance += std::round(glyph.advance);
        }
    };

    float status_text_offset = 25;  // TODO: Convert this magic number to actual code.
    std::string line_str = std::format("Line {}, Column {}", end_caret.line, end_caret.column);

    float y_bottom_of_screen = size.height - main_font_rasterizer.line_height;
    float y_top_of_screen = editor_offset.y / 2 + ui_font_rasterizer.line_height / 2 -
                            main_font_rasterizer.line_height;

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

    glBindTexture(GL_TEXTURE_2D, atlas.tex_id);

    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glCheckError();
}

void TextRenderer::setCaretInfo(Buffer& buffer, FontRasterizer& font_rasterizer, Point& mouse,
                                CaretInfo& caret) {
    float x;
    size_t offset;

    caret.line = mouse.y / font_rasterizer.line_height;
    if (caret.line > buffer.lineCount() - 1) {
        caret.line = buffer.lineCount() - 1;
    }

    std::string start_line_str = buffer.getLineContent(caret.line);
    std::tie(x, offset) = this->closestBoundaryForX(start_line_str, mouse.x, font_rasterizer);
    caret.column = offset;
    caret.x = x;

    caret.byte = buffer.byteOfLine(caret.line) + caret.column;
}

void TextRenderer::moveCaretForwardChar(Buffer& buffer, CaretInfo& caret,
                                        FontRasterizer& main_font_rasterizer) {
    std::string line_str = buffer.getLineContent(caret.line);

    size_t ret = grapheme_next_character_break_utf8(&line_str[0] + caret.column, SIZE_MAX);
    if (ret > 0) {
        std::string_view key = std::string_view(line_str).substr(caret.column, ret);
        AtlasGlyph& glyph = getAtlasGlyph(key, main_font_rasterizer);

        caret.byte += ret;
        caret.column += ret;
        caret.x += std::round(glyph.advance);
    }
}

// TODO: Do we really need a Unicode-accurate version of this? Sublime Text doesn't seem to follow
// Unicode word boundaries the way libgrapheme does.
void TextRenderer::moveCaretForwardWord(Buffer& buffer, CaretInfo& caret,
                                        FontRasterizer& main_font_rasterizer) {
    std::string line_str = buffer.getLineContent(caret.line);

    size_t word_offset = grapheme_next_word_break_utf8(&line_str[0] + caret.column, SIZE_MAX);
    if (word_offset > 0) {
        float total_advance = 0;
        size_t ret;
        for (size_t offset = caret.column; offset < caret.column + word_offset; offset += ret) {
            ret = grapheme_next_character_break_utf8(&line_str[0] + caret.column, SIZE_MAX);
            std::string_view key = std::string_view(line_str).substr(offset, ret);
            AtlasGlyph& glyph = getAtlasGlyph(key, main_font_rasterizer);
            total_advance += std::round(glyph.advance);
        }

        caret.byte += word_offset;
        caret.column += word_offset;
        caret.x += total_advance;
    }
}

TextRenderer::AtlasGlyph TextRenderer::createAtlasGlyph(std::string_view utf8_str,
                                                        FontRasterizer& font_rasterizer) {
#if IS_WIN
    uint_least32_t codepoint = 0;
    grapheme_decode_utf8(&utf8_str[0], SIZE_MAX, &codepoint);
    RasterizedGlyph glyph = font_rasterizer.rasterizeTemp(utf8_str, codepoint);
#else
    RasterizedGlyph glyph = font_rasterizer.rasterizeUTF8(utf8_str);
#endif

    Vec4 uv = atlas.insertTexture(glyph.width, glyph.height, glyph.colored, &glyph.buffer[0]);

    AtlasGlyph atlas_glyph{
        .glyph = Vec4{static_cast<float>(glyph.left), static_cast<float>(glyph.top),
                      static_cast<float>(glyph.width), static_cast<float>(glyph.height)},
        .uv = uv,
        .advance = glyph.advance,
        .colored = glyph.colored,
    };
    return atlas_glyph;
}
}
