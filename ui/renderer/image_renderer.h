#pragma once

#include "ui/renderer/atlas.h"
#include "ui/renderer/shader.h"
#include <glad/glad.h>
#include <vector>

class ImageRenderer {
public:
    ImageRenderer() = default;
    void setup(float width, float height);
    void draw(int width, int height, float scroll_x, float scroll_y, float editor_offset_x,
              float editor_offset_y);
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
