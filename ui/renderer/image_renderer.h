#pragma once

#include "ui/renderer/atlas.h"
#include "ui/renderer/shader.h"
#include <vector>

#include "build/buildflag.h"
#if IS_MAC
#include <OpenGL/gl3.h>
#else
#include <glad/glad.h>
#endif

class ImageRenderer {
public:
    ImageRenderer() = default;
    void setup(float width, float height);
    void draw(float scroll_x, float scroll_y);
    void resize(float new_width, float new_height);
    ~ImageRenderer();

private:
    static const int BATCH_MAX = 65536;

    float width, height;
    Shader shader_program;
    GLuint vao, vbo_instance, ebo;

    Atlas atlas;
    struct AtlasImage {
        Vec2 rect_size;
        Vec4 uv;
    };
    std::vector<AtlasImage> image_atlas_entries;

    bool loadPng(fs::path file_name, int& out_width, int& out_height, bool& out_has_alpha,
                 GLubyte** out_data);
};
