#pragma once

#include "base/buffer.h"
#include "base/rgb.h"
#include "base/syntax_highlighter.h"
#include "config/color_scheme.h"
#include "font/rasterizer.h"
#include "renderer/atlas.h"
#include "renderer/selection_renderer.h"
#include "renderer/shader.h"
#include "renderer/types.h"
#include "simple_text/editor_tab.h"
#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace renderer {
class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();
    void setup(FontRasterizer& font_rasterizer);
    void renderText(Size& size, Point& scroll, Buffer& buffer, SyntaxHighlighter& highlighter,
                    Point& editor_offset, FontRasterizer& font_rasterizer, float status_bar_height,
                    CaretInfo& start_caret, CaretInfo& end_caret, float& longest_line_x,
                    config::ColorScheme& color_scheme, float line_number_offset);
    std::vector<SelectionRenderer::Selection> getSelections(Buffer& buffer,
                                                            FontRasterizer& font_rasterizer,
                                                            CaretInfo& start_caret,
                                                            CaretInfo& end_caret);
    std::vector<float> getTabTitleWidths(Buffer& buffer, FontRasterizer& ui_font_rasterizer,
                                         std::vector<std::unique_ptr<EditorTab>>& editor_tabs);
    void renderUiText(Size& size, FontRasterizer& main_font_rasterizer,
                      FontRasterizer& ui_font_rasterizer, CaretInfo& end_caret,
                      config::ColorScheme& color_scheme, Point& editor_offset,
                      std::vector<std::unique_ptr<EditorTab>>& editor_tabs,
                      std::vector<int>& tab_title_x_coords);
    void setCaretInfo(Buffer& buffer, FontRasterizer& font_rasterizer, Point& mouse,
                      CaretInfo& caret);
    float getGlyphAdvance(std::string utf8_str, FontRasterizer& font_rasterizer);

private:
    static constexpr int kBatchMax = 65536;

    Shader shader_program;
    GLuint vao, vbo_instance, ebo;

    Atlas atlas;
    struct AtlasGlyph {
        Vec4 glyph;
        Vec4 uv;
        float advance;
        bool colored;
    };

    std::vector<std::unordered_map<uint_least32_t, AtlasGlyph>> glyph_cache;

    void loadGlyph(std::string utf8_str, uint_least32_t codepoint,
                   FontRasterizer& font_rasterizer);
    std::pair<float, size_t> closestBoundaryForX(std::string line_str, float x,
                                                 FontRasterizer& font_rasterizer);
};
}
