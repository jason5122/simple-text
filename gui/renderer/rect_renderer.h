#pragma once

#include "gui/renderer/opengl_types.h"
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

    enum class RectLayer {
        kBackground,
        kForeground,
    };

    void addRect(const Point& coords,
                 const Size& size,
                 const Rgba& color,
                 RectLayer rect_type,
                 int corner_radius = 0,
                 int tab_corner_radius = 0);
    void flush(const Size& screen_size, RectLayer rect_type);

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

    std::vector<InstanceData> background_instances;
    std::vector<InstanceData> foreground_instances;
};

static_assert(!std::is_copy_constructible_v<RectRenderer>);
static_assert(!std::is_trivially_copy_constructible_v<RectRenderer>);

}
