#pragma once

#import <OpenGL/gl3.h>

class CursorRenderer {
public:
    CursorRenderer(float width, float height, float cell_width, float cell_height);
    void draw(float scroll_x, float scroll_y);
    void resize(int new_width, int new_height);
    ~CursorRenderer();

private:
    float width, height;
    float cell_width, cell_height;

    GLuint shader_program;
    GLuint vao, vbo, ebo;

    void linkShaders();
};
