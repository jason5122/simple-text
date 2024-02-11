#pragma once

#include "base/buffer.h"
#include "base/syntax_highlighter.h"
// #include "font/core_text_rasterizer.h"
#include "font/freetype_rasterizer.h"
#include "ui/renderer/atlas.h"
#include "ui/renderer/atlas_renderer.h"
#include "ui/renderer/shader.h"
#include <map>
#include <string>
#include <tree_sitter/api.h>

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
    void renderText(Buffer& buffer, float scroll_x, float scroll_y);
    void resize(float new_width, float new_height);
    void setCursorPositions(Buffer& buffer, float cursor_x, float cursor_y, float drag_x,
                            float drag_y);
    void parseBuffer(Buffer& buffer);
    void editBuffer(Buffer& buffer, size_t bytes);
    float getGlyphAdvance(std::string utf8_str);
    ~TextRenderer();

private:
    static const int BATCH_MAX = 65536;

    float width, height;

    GLuint vao, vbo_instance, ebo;

    Shader shader_program;
    Atlas atlas;
    // CoreTextRasterizer ct_rasterizer;
    FreeTypeRasterizer ft_rasterizer;
    AtlasRenderer atlas_renderer;

    std::map<std::string, AtlasGlyph> glyph_cache;

    SyntaxHighlighter highlighter;

    void linkShaders();
    void loadGlyph(std::string utf8_str);
    std::pair<float, size_t> closestBoundaryForX(std::string line_str, float x);
    bool isGlyphInSelection(int row, float glyph_center_x);
    void highlight();
};
