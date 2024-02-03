#pragma once

#include "base/buffer.h"
#include "base/syntax_highlighter.h"
#include "font/rasterizer.h"
#include "third_party/tree_sitter/include/tree_sitter/api.h"
#include "ui/renderer/atlas.h"
#include "ui/renderer/atlas_renderer.h"
#include "ui/renderer/cursor_renderer.h"
#include <OpenGL/gl3.h>
#include <map>
#include <string>

class Renderer {
public:
    size_t cursor_start_line = 0;
    size_t cursor_start_col_offset = 0;
    float cursor_start_x = 0;

    size_t cursor_end_line = 0;
    size_t cursor_end_col_offset = 0;
    float cursor_end_x = 0;

    Renderer(float width, float height, std::string main_font_name, std::string emoji_font_name,
             int font_size, float line_height);
    void renderText(Buffer& buffer, float scroll_x, float scroll_y);
    void resize(int new_width, int new_height);
    void setCursorPositions(Buffer& buffer, float cursor_x, float cursor_y, float drag_x,
                            float drag_y);
    void parseBuffer(Buffer& buffer);
    void editBuffer(Buffer& buffer, size_t bytes);
    float getGlyphAdvance(const char* utf8_str);
    ~Renderer();

private:
    static const int BATCH_MAX = 65536;

    float width, height;
    float line_height;

    GLuint shader_program;
    GLuint vao, vbo_instance, ebo;

    Atlas atlas;
    Rasterizer* rasterizer;
    AtlasRenderer* atlas_renderer;
    CursorRenderer* cursor_renderer;

    std::map<uint32_t, AtlasGlyph> glyph_cache;

    SyntaxHighlighter highlighter;

    void linkShaders();
    void loadGlyph(uint32_t scalar, const char* utf8_str);
    std::pair<float, size_t> closestBoundaryForX(const char* line, float x);
    bool isGlyphInSelection(int row, float glyph_center_x);
    void highlight();
};
