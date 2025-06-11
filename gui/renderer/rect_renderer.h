#pragma once

#include "gl/gl.h"
#include "gui/renderer/shader.h"
#include "gui/renderer/types.h"
#include "gui/types.h"
#include "util/non_copyable.h"
#include <cstddef>
#include <vector>

namespace gui {

class RectRenderer : util::NonCopyable {
public:
    RectRenderer();
    ~RectRenderer();
    RectRenderer(RectRenderer&& other);
    RectRenderer& operator=(RectRenderer&& other);

    void add_rect(const Point& coords,
                  const Size& size,
                  const Point& min_coords,
                  const Point& max_coords,
                  const Rgb& color,
                  Layer layer,
                  int corner_radius = 0,
                  int tab_corner_radius = 0,
                  int left_shadow = 0,
                  int right_shadow = 0);
    void flush(const Size& screen_size, Layer layer);

private:
    static constexpr int kBatchMax = 0x10000;
    static constexpr int kMinTabWidth = 350;

    Shader shader_program;
    gl::GLuint vao = 0;
    gl::GLuint vbo_instance = 0;
    gl::GLuint ebo = 0;

    struct InstanceData {
        Vec2 coords;
        Vec2 rect_size;
        Rgba color;
        // <corner_radius, tab_corner_radius, left_shadow, right_shadow>
        Vec4 extra;
    };

    std::vector<InstanceData> layer_one_instances;
    std::vector<InstanceData> layer_two_instances;
};

}  // namespace gui
