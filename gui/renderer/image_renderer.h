#pragma once

#include "app/types.h"
#include "gui/renderer/shader.h"
#include "gui/renderer/types.h"
#include "util/non_copyable.h"

#include <string_view>
#include <vector>

namespace gui {

class ImageRenderer : util::NonCopyable {
public:
    ImageRenderer();
    ~ImageRenderer();
    ImageRenderer(ImageRenderer&& other);
    ImageRenderer& operator=(ImageRenderer&& other);

    void insertInBatch(size_t image_index, const app::Point& coords, const Rgba& color);
    void renderBatch(const app::Size& screen_size);

private:
    static constexpr size_t kBatchMax = 0x10000;

    Shader shader_program;
    GLuint vao = 0;
    GLuint vbo_instance = 0;
    GLuint ebo = 0;

    struct InstanceData {
        Vec2 coords;
        Vec2 rect_size;
        Vec4 uv;
        Rgba color;
    };

    std::vector<InstanceData> instances;
};

static_assert(!std::is_copy_constructible_v<ImageRenderer>);
static_assert(!std::is_trivially_copy_constructible_v<ImageRenderer>);

}  // namespace gui
