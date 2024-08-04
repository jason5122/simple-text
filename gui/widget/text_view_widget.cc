#include "base/numeric/saturation_arithmetic.h"
#include "gui/renderer/renderer.h"
#include "text_view_widget.h"
#include <cmath>

// TODO: Debug use; remove this.
#include "util/profile_util.h"
#include <iostream>

namespace gui {

TextViewWidget::TextViewWidget(const std::string& text) : table{text}, line_layout_cache{table} {
    updateMaxScroll();
}

void TextViewWidget::selectAll() {
    // start_caret = line_layout.begin();
    // end_caret = std::prev(line_layout.end());
    updateCaretX();
}

void TextViewWidget::move(MoveBy by, bool forward, bool extend) {
    if (by == MoveBy::kCharacters) {
        line_layout_cache.moveByCharacters(forward, end_caret);
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
    table.insert(end_caret.index, text);
    {
        PROFILE_BLOCK("LineLayout::reflow()");
        line_layout_cache.reflow(table, 0);
    }
    updateMaxScroll();
}

void TextViewWidget::leftDelete() {
    scroll_offset.x = 0;

    // GlyphCache& main_glyph_cache = Renderer::instance().getMainGlyphCache();

    // size_t start_byte_offset = (*start_caret).byte_offset;
    // size_t end_byte_offset = (*end_caret).byte_offset;
    // base::Buffer::StringIterator first = buffer.stringBegin() + start_byte_offset;
    // base::Buffer::StringIterator last = buffer.stringBegin() + end_byte_offset;

    // if (first > last) {
    //     std::swap(first, last);
    // }
    // if (first == last && start_caret != line_layout.begin()) {
    //     size_t new_start_byte_offset = (*std::prev(start_caret)).byte_offset;
    //     first = buffer.stringBegin() + new_start_byte_offset;
    // }

    // // Ensure we only move back when 1) there selection is empty, and 2) we are not at the end.
    // bool should_move_back = start_caret == end_caret && end_caret != line_layout.begin() &&
    //                         end_caret != std::prev(line_layout.end());

    // // Store old cursor position before we invalidate our iterators.
    // // TODO: Consider always ensuring `start_caret <= end_caret`.
    // LineLayout::Iterator actual_start = std::min(start_caret, end_caret);
    // size_t old_start_index = line_layout.iteratorIndex(actual_start);

    // buffer.erase(first, last);
    // line_layout.reflow(table, buffer, main_glyph_cache);
    // updateMaxScroll();

    // end_caret = line_layout.getIterator(old_start_index);
    // if (should_move_back) {
    //     std::advance(end_caret, -1);
    // }
    // start_caret = end_caret;
}

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
    start_line = base::sub_sat(start_line, 1UL);
    end_line = base::add_sat(end_line, 1UL);

    start_line = std::clamp(start_line, 0UL, table.lineCount());
    end_line = std::clamp(end_line, 0UL, table.lineCount());

    int min_x = scroll_offset.x;
    int max_x = scroll_offset.x + size.width;
    {
        PROFILE_BLOCK("TextRenderer::renderText()");
        for (size_t line = start_line; line < end_line; ++line) {
            text_renderer.renderLineLayout(position - scroll_offset,
                                           line_layout_cache.getLineLayout(line), line, min_x,
                                           max_x);
        }
    }
    constexpr bool kDebugAtlas = false;
    if constexpr (kDebugAtlas) {
        text_renderer.renderAtlases(position);
    }

    // Add selections.
    bool should_swap = end_caret < start_caret;
    const auto& c1 = should_swap ? end_caret : start_caret;
    const auto& c2 = should_swap ? start_caret : end_caret;
    selection_renderer.renderSelections(position - scroll_offset, line_layout_cache, c1, c2);

    // Add vertical scroll bar.
    int line_count = table.lineCount();
    int line_height = text_renderer.lineHeight();
    int vbar_width = 15;
    int max_scrollbar_y = (line_count + visible_lines) * line_height;
    int vbar_height = size.height * (static_cast<float>(size.height) / max_scrollbar_y);
    float vbar_percent = static_cast<float>(scroll_offset.y) / max_scroll_offset.y;
    Point vbar_coords{
        .x = size.width - vbar_width,
        .y = static_cast<int>(std::round((size.height - vbar_height) * vbar_percent)),
    };
    rect_renderer.addRect(vbar_coords + position, {vbar_width, vbar_height}, scroll_bar_color, 5);

    // Add horizontal scroll bar.
    int hbar_height = 15;
    int hbar_width = size.width * (static_cast<float>(size.width) / max_scroll_offset.x);
    hbar_width = std::max(hbar_width, kMinScrollbarWidth);
    float hbar_percent = static_cast<float>(scroll_offset.x) / max_scroll_offset.x;
    Point hbar_coords{
        .x = static_cast<int>(std::round((size.width - hbar_width) * hbar_percent)),
        .y = size.height - hbar_height,
    };
    rect_renderer.addRect(hbar_coords + position, {hbar_width, hbar_height}, scroll_bar_color, 5);

    // Add caret.
    int caret_width = 4;
    int extra_padding = 8;
    int caret_height = line_height + extra_padding * 2;

    Point caret_pos{
        .x = end_caret.x,
        .y = static_cast<int>(end_caret.line) * line_height,
    };
    caret_pos += position;
    caret_pos -= scroll_offset;
    caret_pos.x -= caret_width / 2;
    caret_pos.y -= extra_padding;
    rect_renderer.addRect(caret_pos, {caret_width, caret_height}, caret_color);
}

void TextViewWidget::leftMouseDown(const Point& mouse_pos) {
    Point new_coords = mouse_pos - position + scroll_offset;
    line_layout_cache.moveToPoint(lineAtPoint(new_coords), new_coords, end_caret);
    start_caret = end_caret;
    updateCaretX();
}

void TextViewWidget::leftMouseDrag(const Point& mouse_pos) {
    Point new_coords = mouse_pos - position + scroll_offset;
    line_layout_cache.moveToPoint(lineAtPoint(new_coords), new_coords, end_caret);
    updateCaretX();
}

void TextViewWidget::updateMaxScroll() {
    TextRenderer& text_renderer = Renderer::instance().getTextRenderer();

    max_scroll_offset.x = line_layout_cache.maxWidth();
    max_scroll_offset.y = table.lineCount() * text_renderer.lineHeight();
}

void TextViewWidget::updateCaretX() {
    // caret_x = (*end_caret).total_advance;
}

size_t TextViewWidget::lineAtPoint(const Point& point) {
    GlyphCache& main_glyph_cache = Renderer::instance().getMainGlyphCache();
    int y = std::max(point.y, 0);
    size_t line = y / main_glyph_cache.lineHeight();
    return std::clamp(line, 0UL, base::sub_sat(table.lineCount(), 1UL));
}

}
