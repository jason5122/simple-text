#pragma once

#include "app/types.h"
#include "gui/renderer/glyph_cache.h"
#include "gui/renderer/shader.h"
#include "gui/renderer/types.h"
#include <functional>
#include <vector>

namespace gui {

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();
    TextRenderer(TextRenderer&& other);
    TextRenderer& operator=(TextRenderer&& other);

    void renderLineLayout(const font::LineLayout& line_layout,
                          const app::Point& coords,
                          Layer layer,
                          const std::function<Rgb(size_t)>& highlight_callback,
                          int min_x = std::numeric_limits<int>::min(),
                          int max_x = std::numeric_limits<int>::max());

    void flush(const app::Size& screen_size, Layer layer);

    // DEBUG: Draws all texture atlases.
    void renderAtlasPages(const app::Point& coords);

private:
    static constexpr size_t kBatchMax = 0x10000;

    GlyphCache glyph_cache;

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

    std::vector<std::vector<InstanceData>> layer_one_instances;
    std::vector<std::vector<InstanceData>> layer_two_instances;

    void insertIntoBatch(size_t page, const InstanceData& instance, Layer layer);
};

}  // namespace gui
