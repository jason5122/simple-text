#pragma once

#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>
#import <glm/glm.hpp>
#import <glm/gtc/matrix_transform.hpp>
#import <glm/gtc/type_ptr.hpp>
#import <map>

struct AtlasGlyph {
    glm::ivec2 size;
    glm::ivec2 bearing;  // Offset from baseline to left/top of glyph.
    float uv_bot;
    float uv_left;
    float uv_width;
    float uv_height;
};

class Renderer {
public:
    Renderer(float width, float height, CTFontRef mainFont);
    void renderText(std::string text, float x, float y);
    void clearAndResize();
    ~Renderer();

private:
    float width, height;
    GLuint shader_program;
    GLuint vao, vbo, vbo_instance, ebo;

    GLuint atlas;
    // https://feedback.wildfiregames.com/report/opengl/feature/GL_MAX_TEXTURE_SIZE
    static const int ATLAS_SIZE = 1024;  // 1024 is a conservative size.

    CTFontRef mainFont;
    std::map<GLchar, AtlasGlyph> glyph_cache;

    void linkShaders();
    void loadGlyphs();
};
