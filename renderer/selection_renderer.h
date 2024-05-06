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
        Vec2 size;
        Rgba color;
        Rgba border_color;
        // <border_flags, bottom_border_offset, top_border_offset, draw_background>
        IVec4 border_info;
    };
    std::vector<InstanceData> instances;
};
}
