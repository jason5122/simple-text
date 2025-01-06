#pragma once

#include "gui/app/types.h"
#include "gui/renderer/shader.h"
#include "gui/renderer/types.h"
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

    void addRect(const Point& coords,
                 const Size& size,
                 const Point& min_coords,
                 const Point& max_coords,
                 const Rgb& color,
                 Layer layer,
                 int corner_radius = 0,
                 int tab_corner_radius = 0);
    void flush(const Size& screen_size, Layer layer);

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
        // The alpha channel of `color` is unused. However, a vec4 is more efficient than a vec3.
        Rgba color;
        float corner_radius = 0;
        float tab_corner_radius = 0;
    };

    std::vector<InstanceData> layer_one_instances;
    std::vector<InstanceData> layer_two_instances;
};

}  // namespace gui
