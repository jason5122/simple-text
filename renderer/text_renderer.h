#pragma once

#include "base/buffer.h"
#include "base/syntax_highlighter.h"
#include "config/color_scheme.h"
#include "renderer/atlas.h"
#include "renderer/glyph_cache.h"
#include "renderer/opengl_types.h"
#include "renderer/selection_renderer.h"
#include "renderer/shader.h"
#include "renderer/types.h"
#include "simple_text/editor_tab.h"
#include <glad/glad.h>
#include <vector>

namespace renderer {
class TextRenderer {
public:
    NOT_COPYABLE(TextRenderer)
    NOT_MOVABLE(TextRenderer)
    TextRenderer(GlyphCache& main_glyph_cache, GlyphCache& ui_glyph_cache);
    ~TextRenderer();
    void setup();
    void renderText(Size& size, Point& scroll, Buffer& buffer, SyntaxHighlighter& highlighter,
                    Point& editor_offset, float status_bar_height, CaretInfo& start_caret,
                    CaretInfo& end_caret, float& longest_line_x, config::ColorScheme& color_scheme,
                    float line_number_offset);
    std::vector<SelectionRenderer::Selection> getSelections(Buffer& buffer, CaretInfo& start_caret,
                                                            CaretInfo& end_caret);
    std::vector<int> getTabTitleWidths(Buffer& buffer,
                                       std::vector<std::unique_ptr<EditorTab>>& editor_tabs);
    void renderUiText(Size& size, CaretInfo& end_caret, config::ColorScheme& color_scheme,
                      Point& editor_offset, std::vector<std::unique_ptr<EditorTab>>& editor_tabs,
                      std::vector<int>& tab_title_x_coords);
    void setCaretInfo(Buffer& buffer, Point& mouse, CaretInfo& caret);
    void moveCaretForwardChar(Buffer& buffer, CaretInfo& caret);
    void moveCaretForwardWord(Buffer& buffer, CaretInfo& caret);

private:
    GlyphCache& main_glyph_cache;
    GlyphCache& ui_glyph_cache;

    static constexpr int kBatchMax = 65536;

    Shader shader_program;
    GLuint vao, vbo_instance, ebo;

    struct InstanceData {
        Vec2 coords;
        Vec4 glyph;
        Vec4 uv;
        Rgba color;
    };

    std::pair<float, size_t> closestBoundaryForX(std::string_view line_str, float x);
};
}
