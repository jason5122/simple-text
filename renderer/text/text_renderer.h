#pragma once

#include "base/buffer.h"
#include "base/syntax_highlighter.h"
#include "config/color_scheme.h"
#include "renderer/opengl_functions.h"
#include "renderer/opengl_types.h"
#include "renderer/selection_renderer.h"
#include "renderer/shader.h"
#include "renderer/text/glyph_cache.h"
#include "renderer/types.h"
#include "simple_text/editor_tab.h"
#include <vector>

namespace renderer {

class TextRenderer {
public:
    TextRenderer(GlyphCache& main_glyph_cache, GlyphCache& ui_glyph_cache);
    ~TextRenderer();
    void setup();
    void renderText(Size& size, Point& scroll, base::Buffer& buffer,
                    base::SyntaxHighlighter& highlighter, Point& editor_offset,
                    CaretInfo& start_caret, CaretInfo& end_caret, int& longest_line_x,
                    config::ColorScheme& color_scheme, int line_number_offset, int& end_caret_x);
    std::vector<SelectionRenderer::Selection>
    getSelections(base::Buffer& buffer, CaretInfo& start_caret, CaretInfo& end_caret);
    std::vector<int> getTabTitleWidths(base::Buffer& buffer,
                                       std::vector<std::unique_ptr<EditorTab>>& editor_tabs);
    void renderUiText(Size& size, CaretInfo& end_caret, config::ColorScheme& color_scheme,
                      Point& editor_offset, std::vector<std::unique_ptr<EditorTab>>& editor_tabs,
                      std::vector<int>& tab_title_x_coords);

private:
    static constexpr size_t kBatchMax = 0x10000;

    GlyphCache& main_glyph_cache;
    GlyphCache& ui_glyph_cache;

    Shader shader_program;
    GLuint vao, vbo_instance, ebo;

    struct InstanceData {
        Vec2 coords;
        Vec4 glyph;
        Vec4 uv;
        Rgba color;
    };

    // TODO: Move batch code into a "Batch" class.
    std::vector<std::vector<InstanceData>> batch_instances;
};

}
