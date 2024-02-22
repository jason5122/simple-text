#pragma once

#include "base/buffer.h"
#include "base/syntax_highlighter.h"
#include "font/core_text_rasterizer.h"
#include "ui/renderer/atlas.h"
#include "ui/renderer/shader.h"
#include <string>
#include <unordered_map>
#include <vector>

#include "build/buildflag.h"
#if IS_MAC
#include <OpenGL/gl3.h>
#else
#include <glad/glad.h>
#endif

class TextRenderer {
public:
    float line_height;

    size_t cursor_start_line = 0;
    size_t cursor_start_col_offset = 0;
    float cursor_start_x = 0;

    size_t cursor_end_line = 0;
    size_t cursor_end_col_offset = 0;
    float cursor_end_x = 0;

    // TODO: Update this during insertion/deletion.
    float longest_line_x = 0;

    TextRenderer() = default;
    void setup(float width, float height, std::string main_font_name, int font_size);
    void renderText(float scroll_x, float scroll_y, Buffer& buffer, SyntaxHighlighter& highlighter,
                    float editor_offset_x, float editor_offset_y);
    void resize(float new_width, float new_height);
    void setCursorPositions(Buffer& buffer, float cursor_x, float cursor_y, float drag_x,
                            float drag_y);
    float getGlyphAdvance(std::string utf8_str);
    ~TextRenderer();

private:
    static const int BATCH_MAX = 65536;

    float width, height;

    GLuint vao, vbo_instance, ebo;

    Shader shader_program;
    Atlas atlas;
    CoreTextRasterizer ct_rasterizer;

    std::unordered_map<uint_least32_t, AtlasGlyph> glyph_cache;

    void loadGlyph(std::string utf8_str, uint_least32_t codepoint);
    std::pair<float, size_t> closestBoundaryForX(std::string line_str, float x);
    bool isGlyphInSelection(int row, float glyph_center_x);
    void highlight();
};
