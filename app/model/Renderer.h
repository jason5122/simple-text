#pragma once

#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>
#import <glm/glm.hpp>
#import <glm/gtc/matrix_transform.hpp>
#import <glm/gtc/type_ptr.hpp>
#import <map>

struct Character {
    GLuint tex_id;       // ID handle of the glyph texture
    glm::ivec2 size;     // size of glyph
    glm::ivec2 bearing;  // Offset from baseline to left/top of glyph
};

class Renderer {
public:
    Renderer(float width, float height, CTFontRef mainFont);
    void render_text(std::string text, float x, float y);
    ~Renderer();

private:
    float width, height;
    GLuint shaderProgram;
    GLuint VAO, VBO, EBO;

    CTFontRef mainFont;
    std::map<GLchar, Character> characters;

    void link_shaders();
    void load_glyphs();
};
