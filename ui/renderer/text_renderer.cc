#include "base/rgb.h"
#include "text_renderer.h"
#include "ui/renderer/opengl_types.h"
#include "util/file_util.h"
#include "util/opengl_error_util.h"
#include "util/profile_util.h"
#include <cmath>
#include <iostream>

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

namespace {
struct InstanceData {
    Vec2 coords;
    Vec4 glyph;
    Vec4 uv;
    Rgba color;
    uint8_t is_atlas = 0;
};
}

void TextRenderer::setup(float width, float height, std::string main_font_name, int font_size) {
    shader_program.link(ResourcePath() / "shaders/text_vert.glsl",
                        ResourcePath() / "shaders/text_frag.glsl");
    this->resize(width, height);

    fs::path font_path = ResourcePath() / "fonts/SourceCodePro-Regular.ttf";

    atlas.setup();

#if IS_MAC
    ct_rasterizer.setup(main_font_name, font_size);
#else
    ct_rasterizer.setup(font_path.c_str(), font_size);
#endif

    this->line_height = ct_rasterizer.line_height;

    glUseProgram(shader_program.id);
    glUniform1f(glGetUniformLocation(shader_program.id, "line_height"), line_height);

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

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

float TextRenderer::getGlyphAdvance(std::string utf8_str) {
    uint_least32_t codepoint;
    grapheme_decode_utf8(&utf8_str[0], SIZE_MAX, &codepoint);

    if (!glyph_cache.count(codepoint)) {
        this->loadGlyph(utf8_str, codepoint);
    }

    AtlasGlyph glyph = glyph_cache[codepoint];
    return std::round(glyph.advance);
}

std::pair<float, size_t> TextRenderer::closestBoundaryForX(std::string line_str, float x) {
    size_t offset;
    size_t ret;
    float total_advance = 0;
    for (offset = 0; offset < line_str.size(); offset += ret) {
        ret = grapheme_next_character_break_utf8(&line_str[0] + offset, SIZE_MAX);

        uint_least32_t codepoint;
        grapheme_decode_utf8(&line_str[0] + offset, SIZE_MAX, &codepoint);

        if (!glyph_cache.count(codepoint)) {
            std::string utf8_str = line_str.substr(offset, ret);
            this->loadGlyph(utf8_str, codepoint);
        }

        AtlasGlyph glyph = glyph_cache[codepoint];

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
                              float editor_offset_y) {
    glUseProgram(shader_program.id);
    glUniform2f(glGetUniformLocation(shader_program.id, "scroll_offset"), scroll_x, scroll_y);
    glUniform2f(glGetUniformLocation(shader_program.id, "editor_offset"), editor_offset_x,
                editor_offset_y);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    size_t start_line = scroll_y / line_height;
    size_t visible_lines = std::ceil((height - 60) / line_height);
    size_t end_line = std::min(start_line + visible_lines, buffer.lineCount());

    {
        PROFILE_BLOCK("Tree-sitter highlight");
        highlighter.getHighlights({static_cast<uint32_t>(start_line), 0},
                                  {static_cast<uint32_t>(end_line), 0});
    }

    size_t byte_offset = buffer.byteOfLine(start_line);
    std::vector<InstanceData> instances;

    {
        PROFILE_BLOCK("layout text");
        for (size_t line_index = start_line; line_index < end_line; line_index++) {
            size_t ret = 1;
            float total_advance = 0;

            std::string line_str;
            buffer.getLineContent(&line_str, line_index);

            for (size_t offset = 0; offset < line_str.size(); offset += ret, byte_offset += ret) {
                ret = grapheme_next_character_break_utf8(&line_str[0] + offset, SIZE_MAX);

                uint_least32_t codepoint;
                grapheme_decode_utf8(&line_str[0] + offset, ret, &codepoint);

                if (!glyph_cache.count(codepoint)) {
                    std::string utf8_str = line_str.substr(offset, ret);
                    this->loadGlyph(utf8_str, codepoint);
                }

                AtlasGlyph glyph = glyph_cache[codepoint];
                Rgb text_color = highlighter.getColor(byte_offset);

                if (total_advance + glyph.advance > scroll_x) {
                    instances.push_back(InstanceData{
                        .coords = Vec2{total_advance, line_index * line_height},
                        .glyph = glyph.glyph,
                        .uv = glyph.uv,
                        .color = Rgba::fromRgb(text_color, glyph.colored),
                    });
                }

                total_advance += std::round(glyph.advance);
            }

            byte_offset++;
            longest_line_x = std::max(total_advance, longest_line_x);
        }
    }

    // instances.push_back(InstanceData{
    //     .coords = Vec2{width - Atlas::ATLAS_SIZE - 400 + scroll_x, 10 * line_height + scroll_y},
    //     .glyph = Vec4{0, 0, Atlas::ATLAS_SIZE, Atlas::ATLAS_SIZE},
    //     .uv = Vec4{0, 0, 1.0, 1.0},
    //     .color = Rgba::fromRgb(colors::black, false),
    //     .is_atlas = true,
    // });

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);

    glBindTexture(GL_TEXTURE_2D, atlas.tex_id);

    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind.
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

void TextRenderer::setCursorPositions(Buffer& buffer, float cursor_x, float cursor_y, float drag_x,
                                      float drag_y) {
    float x;
    size_t offset;

    cursor_start_line = cursor_y / line_height;
    if (cursor_start_line > buffer.lineCount() - 1) {
        std::cerr << "cursor went past last line\n";
        cursor_start_line = buffer.lineCount() - 1;
    }

    std::string start_line_str;
    buffer.getLineContent(&start_line_str, cursor_start_line);
    std::tie(x, offset) = this->closestBoundaryForX(start_line_str, cursor_x);
    cursor_start_col_offset = offset;
    cursor_start_x = x;

    cursor_end_line = drag_y / line_height;
    if (cursor_end_line > buffer.lineCount() - 1) {
        std::cerr << "cursor went past last line\n";
        cursor_end_line = buffer.lineCount() - 1;
    }

    std::string end_line_str;
    buffer.getLineContent(&end_line_str, cursor_end_line);
    std::tie(x, offset) = this->closestBoundaryForX(end_line_str, drag_x);
    cursor_end_col_offset = offset;
    cursor_end_x = x;
}

void TextRenderer::loadGlyph(std::string utf8_str, uint_least32_t codepoint) {
    RasterizedGlyph glyph = ct_rasterizer.rasterizeUTF8(utf8_str.c_str());
    Vec4 uv = atlas.insertTexture(glyph.width, glyph.height, glyph.colored, &glyph.buffer[0]);

    AtlasGlyph atlas_glyph{
        .glyph = Vec4{static_cast<float>(glyph.left), static_cast<float>(glyph.top),
                      static_cast<float>(glyph.width), static_cast<float>(glyph.height)},
        .uv = uv,
        .advance = glyph.advance,
        .colored = glyph.colored,
    };
    glyph_cache.insert({codepoint, atlas_glyph});
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
