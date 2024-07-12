#include "gui/renderer/renderer.h"
#include "text_view_widget.h"
#include <cmath>

// TODO: Debug use; remove this.
#include "util/profile_util.h"
#include <iostream>

namespace gui {

TextViewWidget::TextViewWidget(const Size& size) : ScrollableWidget{size} {}

void TextViewWidget::draw() {
    TextRenderer& text_renderer = Renderer::instance().getTextRenderer();
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();
    SelectionRenderer& selection_renderer = Renderer::instance().getSelectionRenderer();

    constexpr Rgba scroll_bar_color{190, 190, 190, 255};
    constexpr Rgba caret_color{95, 180, 180, 255};

    // Calculate start and end lines.
    size_t visible_lines = std::ceil(static_cast<float>(size.height) / text_renderer.lineHeight());

    size_t start_line = scroll_offset.y / text_renderer.lineHeight();
    size_t end_line = start_line + visible_lines;

    // Render one line before start and one line after end. This ensures no sudden cutoff of
    // rendered text.
    if (start_line > 0) start_line--;                               // Saturating subtraction.
    if (end_line < std::numeric_limits<size_t>::max()) end_line++;  // Saturating addition;

    start_line = std::clamp(start_line, 0UL, buffer.lineCount());
    end_line = std::clamp(end_line, 0UL, buffer.lineCount());

    Point end_caret_pos;
    {
        PROFILE_BLOCK("TextRenderer::renderText()");
        text_renderer.renderText(start_line, end_line, size, position, scroll_offset, buffer,
                                 end_caret, end_caret_pos, longest_line_x);
    }
    updateMaxScroll();  // TODO: Clean this up.

    // Add selections.
    auto selections = selection_renderer.getSelections(
        start_line, end_line, text_renderer.getLineLayout(), start_caret_temp, end_caret_temp);

    Point selection_offset = position - scroll_offset;
    selection_renderer.createInstances(selection_offset, selections);

    // Add vertical scroll bar.
    // int line_count = buffer.lineCount();
    // int line_height = text_renderer.lineHeight();
    // int vbar_width = 15;
    // int visible_lines = std::ceil(static_cast<float>(size.height) / line_height);
    // int max_scrollbar_y = (line_count + visible_lines) * line_height;
    // int vbar_height = size.height * (static_cast<float>(size.height) / max_scrollbar_y);
    // float vbar_percent = static_cast<float>(scroll_offset.y) / max_scroll_offset.y;
    // Point vbar_coords{
    //     .x = size.width - vbar_width,
    //     .y = static_cast<int>(std::round((size.height - vbar_height) * vbar_percent)),
    // };
    // rect_renderer.addRect(vbar_coords + position, {vbar_width, vbar_height}, scroll_bar_color,
    // 5);

    // Add horizontal scroll bar.
    int hbar_height = 15;
    int hbar_width = size.width * (static_cast<float>(size.width) / max_scroll_offset.x);
    float hbar_percent = static_cast<float>(scroll_offset.x) / max_scroll_offset.x;
    Point hbar_coords{
        .x = static_cast<int>(std::round((size.width - hbar_width) * hbar_percent)),
        .y = size.height - hbar_height,
    };
    // rect_renderer.addRect(hbar_coords + position, {hbar_width, hbar_height}, scroll_bar_color,
    // 5);

    // Add caret.
    // int caret_width = 4;
    // int extra_padding = 8;
    // int caret_height = line_height + extra_padding * 2;
    // Point caret_pos{
    //     .x = end_caret_pos.x - caret_width / 2,
    //     .y = end_caret_pos.y - extra_padding,
    // };
    // rect_renderer.addRect(caret_pos, {caret_width, caret_height}, caret_color);
}

void TextViewWidget::leftMouseDown(const Point& mouse_pos) {
    std::cerr << "TextViewWidget::leftMouseDown()\n";

    Movement& movement = Renderer::instance().getMovement();

    Point new_coords = mouse_pos - position + scroll_offset;
    movement.setCaretInfo(buffer, new_coords, end_caret);
    start_caret = end_caret;

    TextRenderer& text_renderer = Renderer::instance().getTextRenderer();
    GlyphCache& main_glyph_cache = Renderer::instance().getMainGlyphCache();

    end_caret_temp =
        text_renderer.getLineLayout().iteratorFromPoint(buffer, main_glyph_cache, new_coords);
    start_caret_temp = end_caret_temp;
}

void TextViewWidget::leftMouseDrag(const Point& mouse_pos) {
    Movement& movement = Renderer::instance().getMovement();

    Point new_coords = mouse_pos - position + scroll_offset;
    movement.setCaretInfo(buffer, new_coords, end_caret);

    TextRenderer& text_renderer = Renderer::instance().getTextRenderer();
    GlyphCache& main_glyph_cache = Renderer::instance().getMainGlyphCache();

    end_caret_temp =
        text_renderer.getLineLayout().iteratorFromPoint(buffer, main_glyph_cache, new_coords);
}

void TextViewWidget::updateMaxScroll() {
    TextRenderer& text_renderer = Renderer::instance().getTextRenderer();
    max_scroll_offset.x = longest_line_x;
    max_scroll_offset.y = buffer.lineCount() * text_renderer.lineHeight();
}

void TextViewWidget::setContents(const std::string& text) {
    TextRenderer& text_renderer = Renderer::instance().getTextRenderer();

    {
        PROFILE_BLOCK("TextRenderer::setContents()");
        buffer.setContents(text);
    }

    {
        PROFILE_BLOCK("TextRenderer::layout()");
        text_renderer.layout(buffer);
    }

    updateMaxScroll();
}

}
