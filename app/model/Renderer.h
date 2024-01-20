#pragma once

#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>
#import <glm/glm.hpp>
#import <glm/gtc/matrix_transform.hpp>
#import <glm/gtc/type_ptr.hpp>
#import <map>

struct AtlasGlyph {
    glm::ivec2 size;     // Size of glyph.
    glm::ivec2 bearing;  // Offset from baseline to left/top of glyph.
    // glm::vec2 uv;        // UV offset for atlas entry.
};

class Renderer {
public:
    Renderer(float width, float height, CTFontRef mainFont);
    void render_text(std::string text, float x, float y);
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

    void link_shaders();
    void load_glyphs();
};
