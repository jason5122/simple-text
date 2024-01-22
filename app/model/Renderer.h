#pragma once

#import "model/Atlas.h"
#import "model/AtlasRenderer.h"
#import "model/GlyphCache.h"
#import "model/Rasterizer.h"
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>
#import <glm/glm.hpp>
#import <glm/gtc/matrix_transform.hpp>
#import <glm/gtc/type_ptr.hpp>
#import <map>
#import <string>
#import <vector>

class Renderer {
public:
    Renderer(float width, float height, std::string main_font_name, std::string emoji_font_name,
             int font_size);
    void renderText(std::vector<std::string> text, float x, float y);
    void clearAndResize();
    ~Renderer();

private:
    float width, height;

    GLuint shader_program;
    GLuint vao, vbo_instance, ebo;
    static const int BATCH_MAX = 65536;

    // https://feedback.wildfiregames.com/report/opengl/feature/GL_MAX_TEXTURE_SIZE
    static const int ATLAS_SIZE = 1024;  // 1024 is a conservative size.

    Atlas atlas;
    Rasterizer* rasterizer;

    GlyphCache glyph_cache2;
    std::map<GLchar, AtlasGlyph> glyph_cache;

    AtlasRenderer* atlas_renderer;

    void linkShaders();
    void loadGlyph(char ch);
};
