#pragma once

#include "font/rasterizer.h"
#include "renderer/opengl_types.h"
#include "renderer/shader.h"
#include "renderer/types.h"
#include <glad/glad.h>

namespace renderer {
class SelectionRenderer {
public:
    struct Selection {
        int line;
        int start;
        int end;
    };

    SelectionRenderer();
    ~SelectionRenderer();
    void setup(FontRasterizer& font_rasterizer);
    void createInstances(Size& size, Point& scroll, Point& editor_offset,
                         FontRasterizer& font_rasterizer, std::vector<Selection>& selections,
                         float line_number_offset);
    void render(int rendering_pass);
    void destroyInstances();

private:
    static constexpr int kBatchMax = 65536;
    static constexpr int kCornerRadius = 6;
    static constexpr int kBorderThickness = 2;

    Shader shader_program;
    GLuint vao, vbo_instance, ebo;

    struct InstanceData {
        Vec2 coords;
        Vec2 bg_size;
        Rgba bg_color;
        Rgba bg_border_color;
        uint32_t border_flags;
        uint32_t bottom_border_offset;
        uint32_t top_border_offset;
    };
    std::vector<InstanceData> instances;
};
}
