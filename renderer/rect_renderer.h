#pragma once

#include "renderer/opengl_types.h"
#include "renderer/shader.h"
#include "renderer/types.h"
#include "util/non_copyable.h"
#include <cstddef>
#include <vector>

namespace renderer {

class RectRenderer : util::NonCopyable {
public:
    RectRenderer();
    ~RectRenderer();
    RectRenderer(RectRenderer&& other);
    RectRenderer& operator=(RectRenderer&& other);

    void draw(const Size& size,
              const Point& scroll,
              const Point& end_caret_pos,
              float line_height,
              size_t line_count,
              float longest_x,
              const Point& editor_offset,
              int status_bar_height);
    void addRect(const Point& coords, const Size& size, Rgba color);
    void addTab(const Point& coords, const Size& size, Rgba color, int tab_corner_radius);
    void flush(const Size& size);

private:
    static constexpr int kBatchMax = 0x10000;
    static constexpr int kMinTabWidth = 350;

    Shader shader_program;
    GLuint vao = 0;
    GLuint vbo_instance = 0;
    GLuint ebo = 0;

    struct InstanceData {
        Vec2 coords;
        Vec2 rect_size;
        Rgba color;
        float corner_radius = 0;
        float tab_corner_radius = 0;
    };

    std::vector<InstanceData> instances;
};

}
