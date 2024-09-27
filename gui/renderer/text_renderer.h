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

    void renderLineLayout(const font::LineLayout& line_layout,
                          const Point& coords,
                          int min_x,
                          int max_x,
                          const Rgb& color,
                          TextLayer font_type);
    void renderLineLayout(const font::LineLayout& line_layout,
                          const Point& coords,
                          int min_x,
                          int max_x,
                          const Rgb& color,
                          TextLayer font_type,
                          const base::SyntaxHighlighter& highlighter,
                          const std::vector<base::SyntaxHighlighter::Highlight>& highlights,
                          size_t line);
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

    void renderLineLayout(const font::LineLayout& line_layout,
                          const Point& coords,
                          int min_x,
                          int max_x,
                          TextLayer font_type,
                          auto compute_color,
                          size_t line);
    void insertIntoBatch(size_t page, const InstanceData& instance, TextLayer font_type);
};

}
