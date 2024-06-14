#pragma once

#include "opengl/functionsgl_typedefs.h"
#include "renderer/opengl_types.h"
#include "renderer/shader.h"
#include "renderer/types.h"
#include "util/non_copyable.h"
#include <cstddef>

namespace renderer {

class RectRenderer : util::NonCopyable {
public:
    RectRenderer(opengl::FunctionsGL* gl);
    ~RectRenderer();
    RectRenderer(RectRenderer&& other);
    RectRenderer& operator=(RectRenderer&& other);

    void draw(const Size& size, const Point& scroll, const CaretInfo& end_caret, int end_caret_x,
              float line_height, size_t line_count, float longest_x, const Point& editor_offset,
              float status_bar_height);

private:
    opengl::FunctionsGL* gl;

    static constexpr int kBatchMax = 65536;
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
};

}
