#pragma once

#include "renderer/atlas.h"
#include "renderer/opengl_types.h"
#include "renderer/shader.h"
#include "renderer/types.h"
#include "util/non_copyable.h"
#include <vector>

#include <filesystem>
namespace fs = std::filesystem;

namespace renderer {

class ImageRenderer : util::NonCopyable {
public:
    ImageRenderer();
    ~ImageRenderer();
    ImageRenderer(ImageRenderer&& other);
    ImageRenderer& operator=(ImageRenderer&& other);

    void addImage();
    void flush(const Size& size);

private:
    static constexpr size_t kBatchMax = 0x10000;

    Shader shader_program;
    GLuint vao = 0;
    GLuint vbo_instance = 0;
    GLuint ebo = 0;

    Atlas atlas;
    struct AtlasImage {
        Vec2 rect_size;
        Vec4 uv;
    };
    std::vector<AtlasImage> image_atlas_entries;

    bool loadPng(fs::path file_name,
                 int& out_width,
                 int& out_height,
                 bool& out_has_alpha,
                 GLubyte** out_data);

    struct InstanceData {
        Vec2 coords;
        Vec2 rect_size;
        Vec4 uv;
        Vec3 color;
    };
    std::vector<InstanceData> instances;
};

}
