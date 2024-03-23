#pragma once

#include "base/buffer.h"
#include "base/rgb.h"
#include "base/syntax_highlighter.h"
#include "font/rasterizer.h"
#include "ui/renderer/atlas.h"
#include "ui/renderer/shader.h"
#include <string>
#include <unordered_map>
#include <vector>

#include "build/buildflag.h"
#if IS_MAC
#include <OpenGL/gl3.h>
#else
#include <epoxy/gl.h>
#endif

class TextRenderer {
public:
    size_t cursor_start_line = 0;
    size_t cursor_start_col_offset = 0;
    float cursor_start_x = 0;

    size_t cursor_end_line = 0;
    size_t cursor_end_col_offset = 0;
    float cursor_end_x = 0;

    // TODO: Update this during insertion/deletion.
    float longest_line_x = 0;

    size_t cursor_start_byte = 0;
    size_t cursor_end_byte = 0;

    TextRenderer() = default;
    void setup(float width, float height, FontRasterizer& font_rasterizer);
    void renderText(float scroll_x, float scroll_y, Buffer& buffer, SyntaxHighlighter& highlighter,
                    float editor_offset_x, float editor_offset_y, FontRasterizer& font_rasterizer,
                    float status_bar_height);
    void renderUiText(FontRasterizer& main_font_rasterizer, FontRasterizer& ui_font_rasterizer);
    void resize(float new_width, float new_height);
    void setCursorPositions(Buffer& buffer, float start_x, float start_y, float end_x, float end_y,
                            FontRasterizer& font_rasterizer);
    float getGlyphAdvance(std::string utf8_str, FontRasterizer& font_rasterizer);
    ~TextRenderer();

private:
    static const int BATCH_MAX = 65536;

    float width, height;
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

    void loadGlyph(std::string utf8_str, uint_least32_t codepoint,
                   FontRasterizer& font_rasterizer);
    std::pair<float, size_t> closestBoundaryForX(std::string line_str, float x,
                                                 FontRasterizer& font_rasterizer);
};
