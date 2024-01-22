#pragma once

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

struct AtlasGlyph {
    bool colored;
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
    float uv_left;
    float uv_bot;
    float uv_width;
    float uv_height;
};

class Renderer {
public:
    Renderer(float width, float height, CTFontRef mainFont);
    void renderText(std::vector<std::string> text, float x, float y);
    void clearAndResize();
    ~Renderer();

private:
    float width, height;

    GLuint shader_program;
    GLuint vao, vbo_instance, ebo;
    static const int BATCH_MAX = 65536;

    GLuint atlas;
    // https://feedback.wildfiregames.com/report/opengl/feature/GL_MAX_TEXTURE_SIZE
    static const int ATLAS_SIZE = 1024;  // 1024 is a conservative size.
    int offset_x, offset_y, tallest;

    Rasterizer rasterizer;

    CTFontRef mainFont;
    CTFontRef emojiFont;
    std::map<GLchar, AtlasGlyph> glyph_cache;

    AtlasRenderer* atlas_renderer;

    void linkShaders();
    void createAtlas();
    void loadGlyph(char ch);
};
