#pragma once

#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

class AtlasRenderer {
public:
    AtlasRenderer(float width, float height);
    void draw(float x, float y, GLuint atlas, int new_width, int new_height);
    ~AtlasRenderer();

private:
    float width, height;

    GLuint shader_program;
    GLuint vao, vbo, ebo;

    void linkShaders();
};
