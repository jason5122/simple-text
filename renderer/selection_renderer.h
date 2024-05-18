#pragma once

#include "font/rasterizer.h"
#include "renderer/opengl_types.h"
#include "renderer/shader.h"
#include "renderer/types.h"
#include "util/not_copyable_or_movable.h"
#include <glad/glad.h>

namespace renderer {
class SelectionRenderer {
public:
    struct Selection {
        int line;
        int start;
        int end;
    };

    NOT_COPYABLE(SelectionRenderer)
    NOT_MOVABLE(SelectionRenderer)
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

    enum BorderFlags {
        kLeft = 1,
        kRight = 1 << 1,
        kBottom = 1 << 2,
        kTop = 1 << 3,
        kBottomLeftInwards = 1 << 4,
        kBottomRightInwards = 1 << 5,
        kTopLeftInwards = 1 << 6,
        kTopRightInwards = 1 << 7,
        kBottomLeftOutwards = 1 << 8,
        kBottomRightOutwards = 1 << 9,
        kTopLeftOutwards = 1 << 10,
        kTopRightOutwards = 1 << 11,
    };

    Shader shader_program;
    GLuint vao, vbo_instance, ebo;

    struct InstanceData {
        IVec2 coords;
        IVec2 size;
        Rgba color;
        Rgba border_color;
        // <border_flags, bottom_border_offset, top_border_offset, hide_background>
        IVec4 border_info;
    };
    std::vector<InstanceData> instances;
};
}
