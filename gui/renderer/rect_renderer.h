#pragma once

#include "gl/gl.h"
#include "gui/renderer/shader.h"
#include "gui/renderer/types.h"
#include "gui/types.h"
#include <cstddef>
#include <vector>

namespace gui {

class RectRenderer {
public:
    RectRenderer();
    ~RectRenderer();
    RectRenderer(const RectRenderer&) = delete;
    RectRenderer& operator=(const RectRenderer&) = delete;
    RectRenderer(RectRenderer&&) noexcept;
    RectRenderer& operator=(RectRenderer&&) noexcept;

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

static_assert(std::movable<RectRenderer>);
static_assert(!std::copyable<RectRenderer>);

}  // namespace gui
