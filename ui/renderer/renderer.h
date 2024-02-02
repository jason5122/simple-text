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
#include <vector>

struct Rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class Renderer {
public:
    int last_cursor_row = 0;
    size_t last_cursor_byte_offset = 0;
    float last_cursor_x = 0;

    int drag_cursor_row = 0;
    size_t drag_cursor_byte_offset = 0;
    float drag_cursor_x = 0;

    Renderer(float width, float height, std::string main_font_name, std::string emoji_font_name,
             int font_size);
    void renderText(Buffer& buffer, float scroll_x, float scroll_y);
    void resize(int new_width, int new_height);
    void setCursorPositions(Buffer& buffer, float cursor_x, float cursor_y, float drag_x,
                            float drag_y);
    void parseBuffer(Buffer& buffer);
    void editBuffer(Buffer& buffer);
    float getGlyphAdvance(const char* utf8_str);
    ~Renderer();

private:
    static const int BATCH_MAX = 65536;
    static constexpr Rgb BLACK{51, 51, 51};
    static constexpr Rgb YELLOW{249, 174, 88};
    static constexpr Rgb BLUE{102, 153, 204};
    static constexpr Rgb GREEN{128, 185, 121};
    static constexpr Rgb RED{236, 95, 102};
    static constexpr Rgb RED2{249, 123, 88};
    static constexpr Rgb GREY2{153, 153, 153};
    static constexpr Rgb PURPLE{198, 149, 198};

    std::vector<std::pair<uint32_t, uint32_t>> highlight_ranges;
    std::vector<Rgb> highlight_colors;

    float width, height;

    GLuint shader_program;
    GLuint vao, vbo_instance, ebo;

    Atlas atlas;
    Rasterizer* rasterizer;
    AtlasRenderer* atlas_renderer;
    CursorRenderer* cursor_renderer;

    std::map<uint32_t, AtlasGlyph> glyph_cache;

    TSTree* tree = NULL;

    SyntaxHighlighter highlighter;

    void linkShaders();
    void loadGlyph(uint32_t scalar, const char* utf8_str);
    std::pair<float, size_t> closestBoundaryForX(const char* line, float x);
    bool isGlyphInSelection(int row, float glyph_center_x);
    void highlight();
};
