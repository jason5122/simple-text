#pragma once

#include "base/syntax_highlighter/syntax_highlighter.h"
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

    enum class TextLayer {
        kBackground,
        kForeground,
    };

    template <typename F>
    void renderLineLayout(const font::LineLayout& line_layout,
                          const Point& coords,
                          TextLayer font_type,
                          F&& highlight_callback,
                          int min_x = std::numeric_limits<int>::min(),
                          int max_x = std::numeric_limits<int>::max()) {
        const auto& font_rasterizer = font::FontRasterizer::instance();
        const auto& metrics = font_rasterizer.getMetrics(line_layout.layout_font_id);

        for (const auto& run : line_layout.runs) {
            for (const auto& glyph : run.glyphs) {
                // If we reach a glyph before the minimum x, skip it and continue.
                // If we reach a glyph *after* the maximum x, break out of the loop — we are done.
                // This assumes glyph positions are monotonically increasing.
                if (glyph.position.x + glyph.advance.x < min_x) {
                    continue;
                }
                if (glyph.position.x > max_x) {
                    break;
                }

                Point glyph_coords = coords;
                glyph_coords.x += glyph.position.x;
                glyph_coords.y += line_layout.ascent;
                glyph_coords.y -= metrics.line_height;

                auto& rglyph = glyph_cache.getGlyph(line_layout.layout_font_id, run.font_id,
                                                    glyph.glyph_id, font_rasterizer);

                // Invert glyph y-offset since we're flipping across the y-axis in OpenGL.
                Vec4 glyph_copy = rglyph.glyph;
                glyph_copy.y = static_cast<float>(metrics.line_height) - glyph_copy.y;

                const InstanceData instance{
                    .coords = glyph_coords.toVec2(),
                    .glyph = glyph_copy,
                    .uv = rglyph.uv,
                    .color = Rgba::fromRgb(highlight_callback(glyph.index), rglyph.colored),
                };
                insertIntoBatch(rglyph.page, std::move(instance), font_type);
            }
        }
    }

    void flush(const Size& screen_size, TextLayer font_type);

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
    std::vector<std::vector<InstanceData>> foreground_batch_instances;
    std::vector<std::vector<InstanceData>> background_batch_instances;

    void insertIntoBatch(size_t page, const InstanceData& instance, TextLayer font_type);
};

}
