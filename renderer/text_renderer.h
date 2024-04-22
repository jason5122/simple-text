#pragma once

#include "base/buffer.h"
#include "base/rgb.h"
#include "base/syntax_highlighter.h"
#include "font/rasterizer.h"
#include "renderer/atlas.h"
#include "renderer/shader.h"
#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <vector>

struct CursorInfo {
    size_t byte;
    size_t line;
    size_t column;
    float x;
    float y;
};

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();
    void setup(FontRasterizer& font_rasterizer);
    void renderText(int width, int height, float scroll_x, float scroll_y, Buffer& buffer,
                    SyntaxHighlighter& highlighter, float editor_offset_x, float editor_offset_y,
                    FontRasterizer& font_rasterizer, float status_bar_height,
                    CursorInfo& start_cursor, CursorInfo& end_cursor, float& longest_line_x);
    void renderUiText(int width, int height, FontRasterizer& main_font_rasterizer,
                      FontRasterizer& ui_font_rasterizer, CursorInfo& end_cursor);
    void setCursorPositions(Buffer& buffer, FontRasterizer& font_rasterizer, float mouse_x,
                            float mouse_y, CursorInfo& cursor);
    float getGlyphAdvance(std::string utf8_str, FontRasterizer& font_rasterizer);

private:
    static const int BATCH_MAX = 65536;

    Shader shader_program;
    GLuint vao, vbo_instance, ebo;

    Atlas atlas;
    struct AtlasGlyph {
        Vec4 glyph;
        Vec4 uv;
        float advance;
        bool colored;
    };

    std::vector<std::unordered_map<uint_least32_t, AtlasGlyph>> glyph_cache;
    std::unordered_map<std::string, std::vector<RasterizedGlyph>> line_layout_cache;

    void loadGlyph(std::string utf8_str, uint_least32_t codepoint,
                   FontRasterizer& font_rasterizer);
    std::pair<float, size_t> closestBoundaryForX(std::string line_str, float x,
                                                 FontRasterizer& font_rasterizer);
};
