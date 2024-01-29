#pragma once

#include "base/buffer.h"
#include "font/rasterizer.h"
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
    Renderer(float width, float height, std::string main_font_name, std::string emoji_font_name,
             int font_size);
    void renderText(Buffer& buffer, float scroll_x, float scroll_y, float cursor_x, float cursor_y,
                    float drag_x, float drag_y);
    void resize(int new_width, int new_height);
    ~Renderer();

private:
    static const int BATCH_MAX = 65536;
    static constexpr Rgb BLACK{51, 51, 51};
    static constexpr Rgb YELLOW{249, 174, 88};
    static constexpr Rgb BLUE{102, 153, 204};
    static constexpr Rgb GREEN{128, 185, 121};
    static constexpr Rgb RED{236, 95, 102};
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

    std::map<GLchar, AtlasGlyph> glyph_cache;
    std::map<uint32_t, AtlasGlyph> glyph_cache2;

    void linkShaders();
    void loadGlyph(char ch);
    void loadGlyph2(uint32_t scalar, const char* utf8_str);
    void treeSitterExperiment();
};
