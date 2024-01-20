#pragma once

#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

class AtlasRenderer {
public:
    AtlasRenderer(float width, float height);
    void draw(float x, float y, GLuint atlas, float atlas_size);
    ~AtlasRenderer();

private:
    float width, height;

    GLuint shader_program;
    GLuint vao, vbo, ebo;

    void linkShaders();
};
