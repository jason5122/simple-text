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
    void renderText(std::vector<std::string> text, float x, float y);
    void clearAndResize();
    ~Renderer();

private:
    static const int BATCH_MAX = 65536;
    static constexpr Rgb BLACK{51, 51, 51};

    float width, height;

    GLuint shader_program;
    GLuint vao, vbo_instance, ebo;

    Atlas atlas;
    Rasterizer* rasterizer;
    AtlasRenderer* atlas_renderer;

    std::map<GLchar, AtlasGlyph> glyph_cache;

    void linkShaders();
    void loadGlyph(char ch);
};
