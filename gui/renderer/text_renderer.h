#pragma once

#include "gui/renderer/glyph_cache.h"
#include "gui/renderer/opengl_types.h"
#include "gui/renderer/shader.h"
#include "gui/renderer/types.h"
#include <vector>

namespace gui {

class TextRenderer {
public:
    TextRenderer(GlyphCache& glyph_cache);
    ~TextRenderer();
    TextRenderer(TextRenderer&& other);
    TextRenderer& operator=(TextRenderer&& other);

    enum class FontType {
        kMain,
        kUI,
    };

    void renderLineLayout(const font::LineLayout& line_layout,
                          const Point& coords,
                          int min_x,
                          int max_x,
                          const Rgb& color,
                          FontType font_type);
    void flush(const Size& screen_size, FontType font_type);

    // DEBUG: Draws all texture atlases.
    void renderAtlasPages(const Point& coords);

private:
    static constexpr size_t kBatchMax = 0x10000;

    GlyphCache& glyph_cache;

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

    // TODO: Move batch code into a "Batch" class.
    std::vector<std::vector<InstanceData>> main_batch_instances;
    std::vector<std::vector<InstanceData>> ui_batch_instances;

    void insertIntoBatch(size_t page, const InstanceData& instance, FontType font_type);
};

}
