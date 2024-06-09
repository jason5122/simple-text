#pragma once

#include "base/filesystem/file_reader.h"
#include "renderer/atlas.h"
#include "renderer/opengl_functions.h"
#include "renderer/opengl_types.h"
#include "renderer/shader.h"
#include "renderer/types.h"
#include "util/non_copyable.h"
#include <vector>

namespace renderer {

class ImageRenderer : util::NonMovable {
public:
    ImageRenderer();
    ~ImageRenderer();
    void setup();
    void draw(Size& size, Point& scroll, Point& editor_offset,
              std::vector<int>& tab_title_x_coords, std::vector<int>& actual_tab_title_widths);

private:
    static constexpr int kBatchMax = 65536;

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

    struct InstanceData {
        Vec2 coords;
        Vec2 rect_size;
        Vec4 uv;
        Vec3 color;
    };
};

}
