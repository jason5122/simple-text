#pragma once

#include "app/types.h"
#include "gui/renderer/glyph_cache.h"
#include "gui/renderer/shader.h"
#include "gui/renderer/types.h"
#include <functional>
#include <vector>

namespace gui {

class TextureRenderer {
public:
    TextureRenderer();
    ~TextureRenderer();
    TextureRenderer(TextureRenderer&& other);
    TextureRenderer& operator=(TextureRenderer&& other);

    void insertLineLayout(const font::LineLayout& line_layout,
                          const app::Point& coords,
                          const std::function<Rgb(size_t)>& highlight_callback,
                          int min_x = std::numeric_limits<int>::min(),
                          int max_x = std::numeric_limits<int>::max(),
                          int min_y = std::numeric_limits<int>::min(),
                          int max_y = std::numeric_limits<int>::max());
    void insertImage(size_t image_index, const app::Point& coords, const Rgba& color);
    void insertColorImage(size_t image_index, const app::Point& coords);
    void flush(const app::Size& screen_size);

private:
    static constexpr size_t kBatchMax = 0x10000;

    Shader shader_program;
    GLuint vao = 0;
    GLuint vbo_instance = 0;
    GLuint ebo = 0;

    struct InstanceData {
        Vec2 coords;
        Vec4 glyph;
        Vec4 uv;
        Rgba color;
    };

    std::vector<std::vector<InstanceData>> batches;

    void insertIntoBatch(size_t page, const InstanceData& instance);

    // DEBUG: Draws texture atlases.
    friend class AtlasWidget;
    void renderAtlasPage(size_t page,
                         const app::Point& coords,
                         int min_x = std::numeric_limits<int>::min(),
                         int max_x = std::numeric_limits<int>::max(),
                         int min_y = std::numeric_limits<int>::min(),
                         int max_y = std::numeric_limits<int>::max());
};

}  // namespace gui
