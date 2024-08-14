#include "base/numeric/literals.h"
#include "base/numeric/saturation_arithmetic.h"
#include "gui/renderer/renderer.h"
#include "text_view_widget.h"
#include <cmath>

// TODO: Debug use; remove this.
#include "util/profile_util.h"
#include <cassert>
#include <iostream>

namespace gui {

TextViewWidget::TextViewWidget(const std::string& text) : table{text} {
    updateMaxScroll();
}

void TextViewWidget::selectAll() {
    // start_caret = line_layout.begin();
    // end_caret = std::prev(line_layout.end());
    // updateCaretX();
}

void TextViewWidget::move(MoveBy by, bool forward, bool extend) {
    size_t line = end_caret.line;
    std::string line_str = table.line(line);
    const auto& layout = line_layout_cache.getLineLayout(line_str);

    if (by == MoveBy::kCharacters && !forward) {
        end_caret.moveToPrevGlyph(layout, line, end_caret.index);
        // updateCaretX();
    }
    if (by == MoveBy::kCharacters && forward) {
        end_caret.moveToNextGlyph(layout, line, end_caret.index);
        // updateCaretX();
    }
    // if (by == MoveBy::kLines) {
    //     end_caret = line_layout.moveByLines(forward, end_caret, caret_x);
    // }

    if (!extend) {
        start_caret = end_caret;
    }
}

void TextViewWidget::moveTo(MoveTo to, bool extend) {
    // size_t line = (*end_caret).line;
    // if (to == MoveTo::kHardBOL) {
    //     end_caret = line_layout.getLine(line);
    //     updateCaretX();
    // }
    // if (to == MoveTo::kHardEOL) {
    //     end_caret = std::prev(line_layout.getLine(line + 1));
    //     updateCaretX();
    // }
    // if (to == MoveTo::kBOF) {
    //     end_caret = line_layout.begin();
    //     updateCaretX();
    // }
    // if (to == MoveTo::kEOF) {
    //     end_caret = std::prev(line_layout.end());
    //     updateCaretX();
    // }

    // if (!extend) {
    //     start_caret = end_caret;
    // }
}

void TextViewWidget::insertText(std::string_view text) {
    table.insert(end_caret.line, end_caret.index, text);

    size_t line = end_caret.line;
    size_t new_index = end_caret.index + text.length();
    std::string line_str = table.line(line);
    const auto& layout = line_layout_cache.getLineLayout(line_str);
    end_caret.moveToIndex(layout, line, new_index);

    start_caret = end_caret;
    // TODO: Do we update caret `max_x` too?

    updateMaxScroll();
}

void TextViewWidget::leftDelete() {
    assert(start_caret <= end_caret);

    // Selection is empty.
    if (start_caret == end_caret) {
        std::string line_str = table.line(end_caret.line);
        const auto& layout = line_layout_cache.getLineLayout(line_str);

        size_t delta = end_caret.moveToPrevGlyph(layout, end_caret.line, end_caret.index);
        table.erase(end_caret.line, end_caret.index, delta);

        start_caret = end_caret;
    } else {
        table.erase(start_caret.line, start_caret.index, end_caret.line, end_caret.index);

        end_caret = start_caret;
    }
}

void TextViewWidget::draw() {
    TextRenderer& text_renderer = Renderer::instance().getTextRenderer();
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();
    SelectionRenderer& selection_renderer = Renderer::instance().getSelectionRenderer();
    const font::FontRasterizer& main_font_rasterizer =
        Renderer::instance().getGlyphCache().mainRasterizer();

    // Calculate start and end lines.
    int main_line_height = main_font_rasterizer.getLineHeight();
    size_t visible_lines = std::ceil(static_cast<float>(size.height) / main_line_height);

    size_t start_line = scroll_offset.y / main_line_height;
    size_t end_line = start_line + visible_lines;

    // Render one line before start and one line after end. This ensures no sudden cutoff of
    // rendered text.
    start_line = base::sub_sat(start_line, 1_Z);
    end_line = base::add_sat(end_line, 1_Z);

    start_line = std::clamp(start_line, 0_Z, table.lineCount());
    end_line = std::clamp(end_line, 0_Z, table.lineCount());

    int min_x = scroll_offset.x;
    int max_x = scroll_offset.x + size.width;
    {
        PROFILE_BLOCK("TextRenderer::renderText()");
        for (size_t line = start_line; line < end_line; ++line) {
            std::string line_str = table.line(line);
            const auto& layout = line_layout_cache.getLineLayout(line_str);
            text_renderer.renderMainLineLayout(position - scroll_offset, layout, line, min_x,
                                               max_x);
        }
    }
    constexpr bool kDebugAtlas = false;
    if constexpr (kDebugAtlas) {
        text_renderer.renderAtlasPages(position);
    }

    // Add selections.
    bool should_swap = end_caret < start_caret;
    const auto& c1 = should_swap ? end_caret : start_caret;
    const auto& c2 = should_swap ? start_caret : end_caret;
    selection_renderer.renderSelections(position - scroll_offset, table, line_layout_cache, c1,
                                        c2);

    // Add vertical scroll bar.
    // int line_count = table.lineCount();
    // int line_height = main_line_height;
    // int vbar_width = 15;
    // int max_scrollbar_y = (line_count + visible_lines) * line_height;
    // int vbar_height = size.height * (static_cast<float>(size.height) / max_scrollbar_y);
    // float vbar_percent = static_cast<float>(scroll_offset.y) / max_scroll_offset.y;
    // Point vbar_coords{
    //     .x = size.width - vbar_width,
    //     .y = static_cast<int>(std::round((size.height - vbar_height) * vbar_percent)),
    // };
    // rect_renderer.addRect(vbar_coords + position, {vbar_width, vbar_height}, kScrollBarColor,
    // 5);

    // Add horizontal scroll bar.
    // int hbar_height = 15;
    // int hbar_width = size.width * (static_cast<float>(size.width) / max_scroll_offset.x);
    // hbar_width = std::max(hbar_width, kMinScrollbarWidth);
    // float hbar_percent = static_cast<float>(scroll_offset.x) / max_scroll_offset.x;
    // Point hbar_coords{
    //     .x = static_cast<int>(std::round((size.width - hbar_width) * hbar_percent)),
    //     .y = size.height - hbar_height,
    // };
    // rect_renderer.addRect(hbar_coords + position, {hbar_width, hbar_height}, kScrollBarColor,
    // 5);

    // Add caret.
    int caret_width = 4;
    int extra_padding = 8;
    int caret_height = main_line_height + extra_padding * 2;

    Point caret_pos{
        .x = end_caret.x,
        .y = static_cast<int>(end_caret.line) * main_line_height,
    };
    caret_pos += position;
    caret_pos -= scroll_offset;
    caret_pos.x -= caret_width / 2;
    caret_pos.y -= extra_padding;
    rect_renderer.addRect(caret_pos, {caret_width, caret_height}, kCaretColor);
}

void TextViewWidget::leftMouseDown(const Point& mouse_pos) {
    leftMouseDrag(mouse_pos);
    start_caret = end_caret;
    // updateCaretX();  // Update for start as well.
}

void TextViewWidget::leftMouseDrag(const Point& mouse_pos) {
    Point new_coords = mouse_pos - position + scroll_offset;
    size_t new_line = lineAtY(new_coords.y);

    std::string line_str = table.line(new_line);
    const auto& layout = line_layout_cache.getLineLayout(line_str);

    end_caret.moveToX(layout, new_line, new_coords.x);
    // updateCaretX();
}

void TextViewWidget::updateMaxScroll() {
    const font::FontRasterizer& main_font_rasterizer =
        Renderer::instance().getGlyphCache().mainRasterizer();

    max_scroll_offset.x = line_layout_cache.maxWidth();
    max_scroll_offset.y = table.lineCount() * main_font_rasterizer.getLineHeight();
}

size_t TextViewWidget::lineAtY(int y) {
    if (y < 0) {
        y = 0;
    }

    const font::FontRasterizer& main_font_rasterizer =
        Renderer::instance().getGlyphCache().mainRasterizer();

    size_t line = y / main_font_rasterizer.getLineHeight();
    return std::clamp(line, 0_Z, base::sub_sat(table.lineCount(), 1_Z));
}

}
