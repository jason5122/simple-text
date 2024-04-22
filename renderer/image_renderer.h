#pragma once

#include "base/filesystem/file_reader.h"
#include "renderer/atlas.h"
#include "renderer/shader.h"
#include "renderer/types.h"
#include <glad/glad.h>
#include <vector>

namespace renderer {
class ImageRenderer {
public:
    ImageRenderer();
    ~ImageRenderer();
    void setup();
    void draw(Size& size, Point& scroll, Point& editor_offset);

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
}