#pragma once

#include "config/color_scheme.h"
#include "renderer/shader.h"
#include "renderer/types.h"
#include <cstddef>
#include <glad/glad.h>

namespace renderer {
class RectRenderer {
public:
    RectRenderer();
    ~RectRenderer();
    void setup();
    void draw(Size& size, Point& scroll, CaretInfo& end_caret, float line_height,
              size_t line_count, float longest_x, Point& editor_offset, float status_bar_height,
              config::ColorScheme& color_scheme, int tab_index, int tab_count);

private:
    static constexpr int kBatchMax = 65536;

    Shader shader_program;
    GLuint vao, vbo_instance, ebo;
};
}
