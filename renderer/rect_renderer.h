#pragma once

#include "config/color_scheme.h"
#include "renderer/shader.h"
#include "renderer/types.h"
#include "util/not_copyable_or_movable.h"
#include <cstddef>
#include <glad/glad.h>
#include <vector>

namespace renderer {
class RectRenderer {
public:
    NOT_COPYABLE(RectRenderer)
    NOT_MOVABLE(RectRenderer)
    RectRenderer();
    ~RectRenderer();
    void setup();
    void draw(Size& size, Point& scroll, CaretInfo& end_caret, float line_height,
              size_t line_count, float longest_x, Point& editor_offset, float status_bar_height,
              config::ColorScheme& color_scheme, int tab_index, std::vector<int>& tab_title_widths,
              float line_number_offset, std::vector<int>& tab_title_x_coords,
              std::vector<int>& actual_tab_title_widths);

private:
    static constexpr int kBatchMax = 65536;
    static constexpr int kMinTabWidth = 350;

    Shader shader_program;
    GLuint vao, vbo_instance, ebo;
};
}
