#pragma once

#import "model/Atlas.h"
#import "model/AtlasRenderer.h"
#import "model/Rasterizer.h"
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>
#import <glm/glm.hpp>
#import <glm/gtc/matrix_transform.hpp>
#import <glm/gtc/type_ptr.hpp>
#import <map>
#import <string>
#import <vector>

struct Rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class Renderer {
public:
    Renderer(float width, float height, std::string main_font_name, std::string emoji_font_name,
             int font_size);
    void renderText(std::vector<std::string> text, float x, float y, uint16_t row_offset);
    void clearAndResize();
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

    uint32_t byte_cutoff = 3000;  // DEBUG: Performance testing.

    std::vector<std::pair<uint32_t, uint32_t>> highlight_ranges;
    std::vector<Rgb> highlight_colors;

    float width, height;

    GLuint shader_program;
    GLuint vao, vbo_instance, ebo;

    Atlas atlas;
    Rasterizer* rasterizer;
    AtlasRenderer* atlas_renderer;

    std::map<GLchar, AtlasGlyph> glyph_cache;

    void linkShaders();
    void loadGlyph(char ch);
    void treeSitterExperiment();
};
