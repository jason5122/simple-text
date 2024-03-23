#include "base/rgb.h"
#include "font/rasterizer.h"
#include "text_renderer.h"
#include "ui/renderer/opengl_types.h"
#include "util/file_util.h"
#include "util/opengl_error_util.h"
#include "util/profile_util.h"
#include <cmath>
#include <cstdint>
#include <iostream>

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

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

void TextRenderer::setup(float width, float height, FontRasterizer& font_rasterizer) {
    shader_program.link(ResourcePath() / "shaders/text_vert.glsl",
                        ResourcePath() / "shaders/text_frag.glsl");
    this->resize(width, height);

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

void TextRenderer::renderText(float scroll_x, float scroll_y, Buffer& buffer,
                              SyntaxHighlighter& highlighter, float editor_offset_x,
                              float editor_offset_y, FontRasterizer& font_rasterizer,
                              float status_bar_height) {
    glUseProgram(shader_program.id);
    glUniform2f(glGetUniformLocation(shader_program.id, "scroll_offset"), scroll_x, scroll_y);
    glUniform2f(glGetUniformLocation(shader_program.id, "editor_offset"), editor_offset_x,
                editor_offset_y);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    size_t start_line = scroll_y / font_rasterizer.line_height;
    size_t visible_lines = std::ceil((height - status_bar_height) / font_rasterizer.line_height);
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

            for (size_t offset = 0; offset < line_str.size(); offset += ret, byte_offset += ret) {
                ret = grapheme_next_character_break_utf8(&line_str[0] + offset, SIZE_MAX);

                uint_least32_t codepoint;
                grapheme_decode_utf8(&line_str[0] + offset, ret, &codepoint);

                if (!glyph_cache[font_rasterizer.id].count(codepoint)) {
                    std::string utf8_str = line_str.substr(offset, ret);
                    this->loadGlyph(utf8_str, codepoint, font_rasterizer);
                }

                AtlasGlyph glyph = glyph_cache[font_rasterizer.id][codepoint];

                // TODO: Determine if we should always round `glyph.advance`.
                Vec2 coords{total_advance, line_index * font_rasterizer.line_height};
                Vec2 bg_size{std::round(glyph.advance), font_rasterizer.line_height};
                if (total_advance + std::round(glyph.advance) > scroll_x) {
                    line_layouts[line_layout_index].emplace_back(codepoint, byte_offset, coords,
                                                                 glyph.glyph, glyph.uv, bg_size,
                                                                 glyph.colored);
                }

                total_advance += std::round(glyph.advance);
            }

            byte_offset++;
            longest_line_x = std::max(total_advance, longest_line_x);
        }
    }

    std::vector<InstanceData> instances;
    for (size_t i = 0; i < line_layouts.size(); i++) {
        for (size_t j = 0; j < line_layouts[i].size(); j++) {
            ShapedGlyph shaped_glyph = line_layouts[i][j];

            float advance = shaped_glyph.bg_size.x;
            float glyph_start_x = shaped_glyph.coords.x;
            float glyph_center_x = glyph_start_x + advance / 2;
            float glyph_end_x = glyph_start_x + advance;

            Rgb text_color = highlighter.getColor(shaped_glyph.byte_offset);
            uint8_t bg_a = this->isGlyphInSelection(start_line + i, glyph_center_x) ? 255 : 0;
            uint8_t border_flags = this->getBorderFlags(glyph_start_x, glyph_end_x);

            instances.push_back(InstanceData{
                .coords = shaped_glyph.coords,
                .glyph = shaped_glyph.glyph,
                .uv = shaped_glyph.uv,
                .color = Rgba::fromRgb(text_color, shaped_glyph.colored),
                .bg_size = shaped_glyph.bg_size,
                .bg_color = Rgba::fromRgb(colors::selection_focused, bg_a),
                .bg_border_color = Rgba::fromRgb(colors::selection_border, border_flags),
            });
        }
    }

    // instances.push_back(InstanceData{
    //     .coords = Vec2{width - Atlas::ATLAS_SIZE - 400 + scroll_x, 10 *
    //     font_rasterizer.line_height + scroll_y}, .glyph = Vec4{0, 0, Atlas::ATLAS_SIZE,
    //     Atlas::ATLAS_SIZE}, .uv = Vec4{0, 0, 1.0, 1.0}, .color = Rgba::fromRgb(colors::black,
    //     false), .is_atlas = true,
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

void TextRenderer::renderUiText(FontRasterizer& main_font_rasterizer,
                                FontRasterizer& ui_font_rasterizer) {
    glUseProgram(shader_program.id);
    glUniform2f(glGetUniformLocation(shader_program.id, "scroll_offset"), 0, 0);
    glUniform2f(glGetUniformLocation(shader_program.id, "editor_offset"), 0, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    float status_text_offset = 25;  // TODO: Convert this magic number to actual code.

    std::vector<InstanceData> instances;

    std::string line_str = "Line 1, Column 1";
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
                           height - main_font_rasterizer.line_height},
            .glyph = glyph.glyph,
            .uv = glyph.uv,
            .color = Rgba::fromRgb(colors::black, glyph.colored),
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

bool TextRenderer::isGlyphInSelection(int row, float glyph_center_x) {
    int start_row, end_row;
    float start_x, end_x;

    if (cursor_start_line == cursor_end_line) {
        if (cursor_start_x <= cursor_end_x) {
            start_row = cursor_start_line;
            end_row = cursor_end_line;
            start_x = cursor_start_x;
            end_x = cursor_end_x;
        } else {
            start_row = cursor_end_line;
            end_row = cursor_start_line;
            start_x = cursor_end_x;
            end_x = cursor_start_x;
        }
    } else if (cursor_start_line < cursor_end_line) {
        start_row = cursor_start_line;
        end_row = cursor_end_line;
        start_x = cursor_start_x;
        end_x = cursor_end_x;
    } else {
        start_row = cursor_end_line;
        end_row = cursor_start_line;
        start_x = cursor_end_x;
        end_x = cursor_start_x;
    }

    if (start_row < row && row < end_row) {
        return true;
    }
    if (start_row == end_row) {
        if (row == start_row && start_x <= glyph_center_x && glyph_center_x <= end_x) {
            return true;
        }
    } else {
        if (row == start_row && start_x <= glyph_center_x) {
            return true;
        }
        if (row == end_row && glyph_center_x <= end_x) {
            return true;
        }
    }
    return false;
}

uint8_t TextRenderer::getBorderFlags(float glyph_start_x, float glyph_end_x) {
    uint8_t border_flags = 0;

    if (cursor_start_line == cursor_end_line) {
        float start_x = cursor_start_x;
        float end_x = cursor_end_x;
        if (cursor_start_x > cursor_end_x) {
            std::swap(start_x, end_x);
        }

        border_flags |= BOTTOM | TOP;
        if (glyph_start_x == start_x) {
            border_flags |= LEFT | BOTTOM_LEFT | TOP_LEFT;
        }
        if (glyph_end_x == end_x) {
            border_flags |= RIGHT | BOTTOM_RIGHT | TOP_RIGHT;
        }
    }
    return border_flags;
}

void TextRenderer::setCursorPositions(Buffer& buffer, float cursor_x, float cursor_y, float drag_x,
                                      float drag_y, FontRasterizer& font_rasterizer) {
    float x;
    size_t offset;

    cursor_start_line = cursor_y / font_rasterizer.line_height;
    if (cursor_start_line > buffer.lineCount() - 1) {
        cursor_start_line = buffer.lineCount() - 1;
    }

    std::string start_line_str;
    buffer.getLineContent(&start_line_str, cursor_start_line);
    std::tie(x, offset) = this->closestBoundaryForX(start_line_str, cursor_x, font_rasterizer);
    cursor_start_col_offset = offset;
    cursor_start_x = x;

    cursor_end_line = drag_y / font_rasterizer.line_height;
    if (cursor_end_line > buffer.lineCount() - 1) {
        cursor_end_line = buffer.lineCount() - 1;
    }

    std::string end_line_str;
    buffer.getLineContent(&end_line_str, cursor_end_line);
    std::tie(x, offset) = this->closestBoundaryForX(end_line_str, drag_x, font_rasterizer);
    cursor_end_col_offset = offset;
    cursor_end_x = x;
}

void TextRenderer::loadGlyph(std::string utf8_str, uint_least32_t codepoint,
                             FontRasterizer& font_rasterizer) {
    RasterizedGlyph glyph = font_rasterizer.rasterizeUTF8(utf8_str.c_str());
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

void TextRenderer::resize(float new_width, float new_height) {
    width = new_width;
    height = new_height;

    glViewport(0, 0, width, height);
    glUseProgram(shader_program.id);
    glUniform2f(glGetUniformLocation(shader_program.id, "resolution"), width, height);
}

TextRenderer::~TextRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo_instance);
    glDeleteBuffers(1, &ebo);
}
