#pragma once

#include "base/buffer.h"
#include "base/syntax_highlighter.h"
#include "config/color_scheme.h"
#include "font/rasterizer.h"
#include "renderer/atlas.h"
#include "renderer/glyph_cache.h"
#include "renderer/opengl_types.h"
#include "renderer/selection_renderer.h"
#include "renderer/shader.h"
#include "renderer/types.h"
#include "simple_text/editor_tab.h"
#include <array>
#include <glad/glad.h>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace renderer {
class TextRenderer {
public:
    NOT_COPYABLE(TextRenderer)
    NOT_MOVABLE(TextRenderer)
    TextRenderer(GlyphCache& main_glyph_cache, GlyphCache& ui_glyph_cache);
    ~TextRenderer();
    void setup(font::FontRasterizer& font_rasterizer);
    void renderText(Size& size, Point& scroll, Buffer& buffer, SyntaxHighlighter& highlighter,
                    Point& editor_offset, font::FontRasterizer& font_rasterizer,
                    float status_bar_height, CaretInfo& start_caret, CaretInfo& end_caret,
                    float& longest_line_x, config::ColorScheme& color_scheme,
                    float line_number_offset);
    std::vector<SelectionRenderer::Selection> getSelections(Buffer& buffer,
                                                            font::FontRasterizer& font_rasterizer,
                                                            CaretInfo& start_caret,
                                                            CaretInfo& end_caret);
    std::vector<int> getTabTitleWidths(Buffer& buffer, font::FontRasterizer& ui_font_rasterizer,
                                       std::vector<std::unique_ptr<EditorTab>>& editor_tabs);
    void renderUiText(Size& size, font::FontRasterizer& main_font_rasterizer,
                      font::FontRasterizer& ui_font_rasterizer, CaretInfo& end_caret,
                      config::ColorScheme& color_scheme, Point& editor_offset,
                      std::vector<std::unique_ptr<EditorTab>>& editor_tabs,
                      std::vector<int>& tab_title_x_coords);
    void setCaretInfo(Buffer& buffer, font::FontRasterizer& font_rasterizer, Point& mouse,
                      CaretInfo& caret);
    void moveCaretForwardChar(Buffer& buffer, CaretInfo& caret,
                              font::FontRasterizer& main_font_rasterizer);
    void moveCaretForwardWord(Buffer& buffer, CaretInfo& caret,
                              font::FontRasterizer& main_font_rasterizer);

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

    std::pair<float, size_t> closestBoundaryForX(std::string_view line_str, float x,
                                                 font::FontRasterizer& font_rasterizer);
    AtlasGlyph& getAtlasGlyph(std::string_view key, bool use_main);
};
}
