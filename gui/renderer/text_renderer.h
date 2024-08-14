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

    void renderMainLineLayout(const Point& offset,
                              const font::LineLayout& line_layout,
                              size_t line,
                              int min_x,
                              int max_x);
    void renderUILineLayout(const Point& coords,
                            const Rgb& color,
                            const font::LineLayout& line_layout);
    void flush(const Size& screen_size, bool use_main_glyph_cache);
    int lineHeight();
    int uiLineHeight();

    // DEBUG: Draws all texture atlases.
    void renderAtlases(const Point& coords);

private:
    static constexpr size_t kBatchMax = 0x10000;

    GlyphCache& glyph_cache;

    Shader shader_program;
    GLuint vao, vbo_instance, ebo;

    struct InstanceData {
        Vec2 coords;
        Vec4 glyph;
        Vec4 uv;
        Rgba color;
    };

    // TODO: Move batch code into a "Batch" class.
    std::vector<std::vector<InstanceData>> main_batch_instances;
    std::vector<std::vector<InstanceData>> ui_batch_instances;

    void insertIntoBatch(size_t page, const InstanceData& instance, bool use_main_glyph_cache);
};

}
