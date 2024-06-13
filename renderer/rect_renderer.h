#pragma once

// #include "config/color_scheme.h"
#include "opengl/functionsgl_typedefs.h"
#include "renderer/opengl_types.h"
#include "renderer/shader.h"
#include "renderer/types.h"
#include "util/non_copyable.h"
#include <cstddef>
#include <vector>

namespace renderer {

class RectRenderer {
public:
    RectRenderer(opengl::FunctionsGL* gl);
    ~RectRenderer();
    void setup();
    // void draw(Size& size, Point& scroll, CaretInfo& end_caret, int end_caret_x, float
    // line_height,
    //           size_t line_count, float longest_x, Point& editor_offset, float status_bar_height,
    //           config::ColorScheme& color_scheme, int tab_index, std::vector<int>&
    //           tab_title_widths, float line_number_offset, std::vector<int>& tab_title_x_coords,
    //           std::vector<int>& actual_tab_title_widths);

private:
    opengl::FunctionsGL* gl;

    static constexpr int kBatchMax = 65536;
    static constexpr int kMinTabWidth = 350;

    Shader shader_program;
    GLuint vao, vbo_instance, ebo;

    struct InstanceData {
        Vec2 coords;
        Vec2 rect_size;
        Rgba color;
        float corner_radius = 0;
        float tab_corner_radius = 0;
    };
};

}
