#include "base/filesystem/file_reader.h"
#include "base/rgb.h"
#include "font/rasterizer.h"
#include "renderer/opengl_error_util.h"
#include "renderer/opengl_types.h"
#include "text_renderer.h"
#include "util/profile_util.h"
#include <cmath>
#include <cstdint>
#include <iostream>

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

#include "build/buildflag.h"

namespace renderer {
// TODO: Rewrite this in a more idiomatic C++ way.
// Border flags.
#define LEFT 1
#define RIGHT 2
#define BOTTOM 4
#define TOP 8
#define BOTTOM_LEFT 16
#define BOTTOM_RIGHT 32
#define TOP_LEFT 64
#define TOP_RIGHT 128

namespace {
struct InstanceData {
    Vec2 coords;
    Vec4 glyph;
    Vec4 uv;
    Rgba color;
    uint8_t is_atlas = 0;
    Vec2 bg_size;
    Rgba bg_color;
    Rgba bg_border_color;
};

// Reference Zed's implementation in //crates/gpui/src/text_system/line_layout.rs.
struct ShapedGlyph {
    uint32_t codepoint;
    size_t byte_offset;  // Byte offset within the line layout.
    Vec2 coords;
    Vec4 glyph;
    Vec4 uv;
    Vec2 bg_size;
    bool colored;
};
}

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
    glyph_cache.emplace_back();
    glyph_cache.emplace_back();

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
    glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData) * BATCH_MAX, nullptr, GL_STATIC_DRAW);

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

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, is_atlas));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, bg_size));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, bg_color));
    glVertexAttribDivisor(index++, 1);

    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(InstanceData),
                          (void*)offsetof(InstanceData, bg_border_color));
    glVertexAttribDivisor(index++, 1);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

float TextRenderer::getGlyphAdvance(std::string utf8_str, FontRasterizer& font_rasterizer) {
    uint_least32_t codepoint;
    grapheme_decode_utf8(&utf8_str[0], SIZE_MAX, &codepoint);

    if (!glyph_cache[font_rasterizer.id].count(codepoint)) {
        this->loadGlyph(utf8_str, codepoint, font_rasterizer);
    }

    AtlasGlyph glyph = glyph_cache[font_rasterizer.id][codepoint];
    return std::round(glyph.advance);
}

// TODO: Rewrite this so this operates on an already shaped line.
//       We should remove any glyph cache/font rasterization from this method.
std::pair<float, size_t> TextRenderer::closestBoundaryForX(std::string line_str, float x,
                                                           FontRasterizer& font_rasterizer) {
    size_t offset;
    size_t ret;
    float total_advance = 0;
    for (offset = 0; offset < line_str.size(); offset += ret) {
        ret = grapheme_next_character_break_utf8(&line_str[0] + offset, SIZE_MAX);

        uint_least32_t codepoint;
        grapheme_decode_utf8(&line_str[0] + offset, SIZE_MAX, &codepoint);

        if (!glyph_cache[font_rasterizer.id].count(codepoint)) {
            std::string utf8_str = line_str.substr(offset, ret);
            this->loadGlyph(utf8_str, codepoint, font_rasterizer);
        }

        AtlasGlyph glyph = glyph_cache[font_rasterizer.id][codepoint];

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
                              CursorInfo& start_cursor, CursorInfo& end_cursor,
                              float& longest_line_x, config::ColorScheme& color_scheme) {
    glUseProgram(shader_program.id);
    glUniform2f(glGetUniformLocation(shader_program.id, "resolution"), size.width, size.height);
    glUniform2f(glGetUniformLocation(shader_program.id, "scroll_offset"), scroll.x, scroll.y);
    glUniform2f(glGetUniformLocation(shader_program.id, "editor_offset"), editor_offset.x,
                editor_offset.y);

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

    std::vector<std::vector<ShapedGlyph>> line_layouts;

    {
        PROFILE_BLOCK("layout text");
        for (size_t line_index = start_line, line_layout_index = 0; line_index < end_line;
             line_index++, line_layout_index++) {
            line_layouts.emplace_back();

            size_t ret;
            float total_advance = 0;

            std::string line_str;
            buffer.getLineContent(&line_str, line_index);

            // Debugging purposes.
            bool disable_cache = false;

            for (size_t offset = 0; offset < line_str.size(); offset += ret, byte_offset += ret) {
                ret = grapheme_next_character_break_utf8(&line_str[0] + offset, SIZE_MAX);

                uint_least32_t codepoint;
                grapheme_decode_utf8(&line_str[0] + offset, ret, &codepoint);

                // If a space character is in selection, draw it as a visible symbol.
                // FIXME: Don't use magic numbers here.
                size_t selection_start = start_cursor.byte;
                size_t selection_end = end_cursor.byte;
                if (selection_start > selection_end) {
                    std::swap(selection_start, selection_end);
                }

                // TODO: Preserve the width of the space character when substituting.
                //       Otherwise, the line width changes when using proportional fonts.
                if (codepoint == 0x20 && selection_start <= byte_offset &&
                    byte_offset < selection_end) {
                    // codepoint = 183;
                }

                if (disable_cache || !glyph_cache[font_rasterizer.id].count(codepoint)) {
                    std::string utf8_str = line_str.substr(offset, ret);

                    // FIXME: Don't use magic numbers here.
                    if (codepoint == 183) {
                        utf8_str = "·";
                    }

                    this->loadGlyph(utf8_str, codepoint, font_rasterizer);
                }

                AtlasGlyph& glyph = glyph_cache[font_rasterizer.id][codepoint];

                // TODO: Determine if we should always round `glyph.advance`.
                Vec2 coords{total_advance, line_index * font_rasterizer.line_height};
                Vec2 bg_size{std::round(glyph.advance), font_rasterizer.line_height};
                if (total_advance + std::round(glyph.advance) > scroll.x) {
                    line_layouts[line_layout_index].emplace_back(
                        ShapedGlyph{codepoint, byte_offset, coords, glyph.glyph, glyph.uv, bg_size,
                                    glyph.colored});
                }

                total_advance += std::round(glyph.advance);
            }

            byte_offset++;
            longest_line_x = std::max(total_advance, longest_line_x);
        }
    }

    size_t selection_start = start_cursor.byte;
    size_t selection_end = end_cursor.byte;
    size_t selection_start_line = start_cursor.line;
    size_t selection_end_line = end_cursor.line;
    float selection_start_x = start_cursor.x;
    float selection_end_x = end_cursor.x;
    if (selection_start > selection_end) {
        std::swap(selection_start, selection_end);
        std::swap(selection_start_line, selection_end_line);
        std::swap(selection_start_x, selection_end_x);
    }

    float prev_line_start_x = 0;
    float prev_line_end_x = 0;
    float next_line_end_x = 0;

    std::vector<InstanceData> instances;
    for (size_t i = 0; i < line_layouts.size(); i++) {
        if (i + 1 < line_layouts.size() && !line_layouts[i + 1].empty()) {
            if (i + 1 == selection_end_line) {
                next_line_end_x = selection_end_x;
            } else {
                ShapedGlyph last_glyph = line_layouts[i + 1].back();
                float glyph_end_x = last_glyph.coords.x + last_glyph.bg_size.x;
                next_line_end_x = glyph_end_x;
            }
        } else {
            next_line_end_x = 0;
        }

        // if (i == selection_start_line) {
        //     prev_line_end_x = 0;
        // }

        for (size_t j = 0; j < line_layouts[i].size(); j++) {
            ShapedGlyph& shaped_glyph = line_layouts[i][j];

            float advance = shaped_glyph.bg_size.x;
            float glyph_start_x = shaped_glyph.coords.x;
            float glyph_center_x = glyph_start_x + advance / 2;
            float glyph_end_x = glyph_start_x + advance;

            bool is_glyph_in_selection = selection_start <= shaped_glyph.byte_offset &&
                                         shaped_glyph.byte_offset < selection_end;

            Rgb text_color = highlighter.getColor(shaped_glyph.byte_offset, color_scheme);
            if (shaped_glyph.codepoint == 183) {
                text_color = Rgb{182, 182, 182};
            }

            uint8_t bg_a = is_glyph_in_selection ? 255 : 0;

            uint8_t border_flags = 0;
            size_t line_index = start_line + i;
            if (line_index == selection_start_line) {
                border_flags |= TOP;
            }
            if (line_index == selection_end_line) {
                border_flags |= BOTTOM;
            }
            if (shaped_glyph.byte_offset == selection_start) {
                border_flags |= LEFT;
                border_flags |= TOP_LEFT;
            }
            if (shaped_glyph.byte_offset == selection_end - 1) {
                border_flags |= RIGHT;
                border_flags |= BOTTOM_RIGHT;
                if (glyph_end_x > prev_line_end_x) {
                    border_flags |= TOP_RIGHT;
                }
            }
            if (j == 0) {
                border_flags |= LEFT;
                if (glyph_start_x < prev_line_start_x) {
                    border_flags |= TOP_LEFT;
                }
                if (line_index == selection_end_line) {
                    border_flags |= BOTTOM_LEFT;
                }
            }
            if (j == line_layouts[i].size() - 1) {
                border_flags |= RIGHT;
                if (glyph_start_x >= prev_line_end_x) {
                    border_flags |= TOP_RIGHT;
                }
                if (glyph_start_x >= next_line_end_x) {
                    border_flags |= BOTTOM_RIGHT;
                }
            }

            if (glyph_start_x >= prev_line_end_x) {
                border_flags |= TOP;
            }
            if (glyph_start_x >= next_line_end_x) {
                border_flags |= BOTTOM;
            }

            if (line_index == selection_start_line + 1 && glyph_end_x <= selection_start_x) {
                border_flags |= TOP;
            }
            if (line_index == selection_end_line - 1 && glyph_start_x >= selection_end_x) {
                border_flags |= BOTTOM;
            }

            instances.push_back(InstanceData{
                .coords = shaped_glyph.coords,
                .glyph = shaped_glyph.glyph,
                .uv = shaped_glyph.uv,
                .color = Rgba::fromRgb(text_color, shaped_glyph.colored),
                .bg_size = shaped_glyph.bg_size,
                .bg_color = Rgba::fromRgb(colors::selection_unfocused, bg_a),
                // .bg_border_color = Rgba::fromRgb(colors::selection_border, border_flags),
                .bg_border_color = Rgba::fromRgb(colors::purple, border_flags),
            });
        }

        if (!line_layouts[i].empty()) {
            ShapedGlyph last_glyph = line_layouts[i].back();
            float glyph_end_x = last_glyph.coords.x + last_glyph.bg_size.x;
            prev_line_end_x = glyph_end_x;
        } else {
            prev_line_end_x = 0;
        }

        if (i == selection_start_line) {
            prev_line_start_x = selection_start_x;
        } else {
            prev_line_start_x = 0;
        }

        if (line_layouts[i].empty()) {
            prev_line_start_x = std::numeric_limits<float>::max();
        }
    }

    // instances.push_back(InstanceData{
    //     .coords = Vec2{width - Atlas::ATLAS_SIZE - 400 + scroll_x,
    //                    10 * font_rasterizer.line_height + scroll_y},
    //     .glyph = Vec4{0, 0, Atlas::ATLAS_SIZE, Atlas::ATLAS_SIZE},
    //     .uv = Vec4{0, 0, 1.0, 1.0},
    //     .color = Rgba::fromRgb(color_scheme.foreground, false),
    //     .is_atlas = true,
    // });

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);

    glBindTexture(GL_TEXTURE_2D, atlas.tex_id);

    glUniform1i(glGetUniformLocation(shader_program.id, "rendering_pass"), 0);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());
    glUniform1i(glGetUniformLocation(shader_program.id, "rendering_pass"), 1);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind.
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glCheckError();
}

void TextRenderer::renderUiText(Size& size, FontRasterizer& main_font_rasterizer,
                                FontRasterizer& ui_font_rasterizer, CursorInfo& end_cursor,
                                config::ColorScheme& color_scheme) {
    glUseProgram(shader_program.id);
    glUniform2f(glGetUniformLocation(shader_program.id, "resolution"), size.width, size.height);
    glUniform2f(glGetUniformLocation(shader_program.id, "scroll_offset"), 0, 0);
    glUniform2f(glGetUniformLocation(shader_program.id, "editor_offset"), 0, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    float status_text_offset = 25;  // TODO: Convert this magic number to actual code.

    std::vector<InstanceData> instances;

    std::string line_str = "Line " + std::to_string(end_cursor.line) + ", Column " +
                           std::to_string(end_cursor.column);
    size_t ret;
    float total_advance = 0;
    for (size_t offset = 0; offset < line_str.size(); offset += ret) {
        ret = grapheme_next_character_break_utf8(&line_str[0] + offset, SIZE_MAX);

        uint_least32_t codepoint;
        grapheme_decode_utf8(&line_str[0] + offset, ret, &codepoint);

        if (!glyph_cache[ui_font_rasterizer.id].count(codepoint)) {
            std::string utf8_str = line_str.substr(offset, ret);
            this->loadGlyph(utf8_str, codepoint, ui_font_rasterizer);
        }

        AtlasGlyph glyph = glyph_cache[ui_font_rasterizer.id][codepoint];

        instances.push_back(InstanceData{
            .coords = Vec2{total_advance + status_text_offset,
                           size.height - main_font_rasterizer.line_height},
            .glyph = glyph.glyph,
            .uv = glyph.uv,
            .color = Rgba::fromRgb(color_scheme.foreground, glyph.colored),
        });

        total_advance += std::round(glyph.advance);
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);

    glBindTexture(GL_TEXTURE_2D, atlas.tex_id);

    glUniform1i(glGetUniformLocation(shader_program.id, "rendering_pass"), 1);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glCheckError();
}

void TextRenderer::setCursorInfo(Buffer& buffer, FontRasterizer& font_rasterizer, Point& mouse,
                                 CursorInfo& cursor) {
    float x;
    size_t offset;

    cursor.line = mouse.y / font_rasterizer.line_height;
    if (cursor.line > buffer.lineCount() - 1) {
        cursor.line = buffer.lineCount() - 1;
    }

    std::string start_line_str;
    buffer.getLineContent(&start_line_str, cursor.line);
    std::tie(x, offset) = this->closestBoundaryForX(start_line_str, mouse.x, font_rasterizer);
    cursor.column = offset;
    cursor.x = x;

    cursor.byte = buffer.byteOfLine(cursor.line) + cursor.column;
}

void TextRenderer::loadGlyph(std::string utf8_str, uint_least32_t codepoint,
                             FontRasterizer& font_rasterizer) {
#if IS_WIN
    RasterizedGlyph glyph = font_rasterizer.rasterizeTemp(utf8_str, codepoint);
#else
    RasterizedGlyph glyph = font_rasterizer.rasterizeUTF8(utf8_str.c_str());
#endif

    Vec4 uv = atlas.insertTexture(glyph.width, glyph.height, glyph.colored, &glyph.buffer[0]);

    AtlasGlyph atlas_glyph{
        .glyph = Vec4{static_cast<float>(glyph.left), static_cast<float>(glyph.top),
                      static_cast<float>(glyph.width), static_cast<float>(glyph.height)},
        .uv = uv,
        .advance = glyph.advance,
        .colored = glyph.colored,
    };
    glyph_cache[font_rasterizer.id].insert({codepoint, atlas_glyph});
}
}
