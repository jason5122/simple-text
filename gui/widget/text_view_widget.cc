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
    size_t start_line = 0;
    size_t end_line = base::sub_sat(table.lineCount(), 1_Z);
    const auto& start_layout = layoutAt(start_line);
    const auto& end_layout = layoutAt(end_line);

    start_caret.moveToIndex(start_layout, start_line, 0);
    end_caret.moveToIndex(end_layout, end_line, end_layout.length);
    // updateCaretX();
}

void TextViewWidget::move(MoveBy by, bool forward, bool extend) {
    size_t line = end_caret.line;
    bool exclude_end;
    const auto& layout = layoutAt(line, exclude_end);

    if (by == MoveBy::kCharacters && !forward) {
        end_caret.moveToPrevGlyph(layout, line, end_caret.index);
        // updateCaretX();
    }
    if (by == MoveBy::kCharacters && forward) {
        end_caret.moveToNextGlyph(layout, line, end_caret.index, exclude_end);
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
    if (to == MoveTo::kHardBOL) {
        bool exclude_end;
        const auto& layout = layoutAt(end_caret.line, exclude_end);
        end_caret.moveToIndex(layout, end_caret.line, 0, exclude_end);
        // updateCaretX();
    }
    if (to == MoveTo::kHardEOL) {
        bool exclude_end;
        const auto& layout = layoutAt(end_caret.line, exclude_end);
        end_caret.moveToIndex(layout, end_caret.line, layout.length, exclude_end);
        // updateCaretX();
    }
    if (to == MoveTo::kBOF) {
        size_t start_line = 0;
        const auto& layout = layoutAt(start_line);
        end_caret.moveToIndex(layout, start_line, 0);
        // updateCaretX();
    }
    if (to == MoveTo::kEOF) {
        size_t end_line = base::sub_sat(table.lineCount(), 1_Z);
        const auto& layout = layoutAt(end_line);
        end_caret.moveToIndex(layout, end_line, layout.length);
        // updateCaretX();
    }

    if (!extend) {
        start_caret = end_caret;
    }
}

void TextViewWidget::insertText(std::string_view text) {
    table.insert(end_caret.line, end_caret.index, text);

    size_t line = end_caret.line;
    size_t new_index = end_caret.index + text.length();
    bool exclude_end;
    const auto& layout = layoutAt(line, exclude_end);
    end_caret.moveToIndex(layout, line, new_index, exclude_end);

    start_caret = end_caret;
    // TODO: Do we update caret `max_x` too?

    updateMaxScroll();
}

void TextViewWidget::leftDelete() {
    assert(start_caret <= end_caret);

    // Selection is empty.
    if (start_caret == end_caret) {
        const auto& layout = layoutAt(end_caret.line);

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

    const auto& glyph_cache = Renderer::instance().getGlyphCache();
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.getMetrics(glyph_cache.mainFontId());

    // Calculate start and end lines.
    int main_line_height = metrics.line_height;
    size_t visible_lines = std::ceil(static_cast<float>(size.height) / main_line_height);

    size_t start_line = scroll_offset.y / main_line_height;
    size_t end_line = start_line + visible_lines;

    // Render one line before start and one line after end. This ensures no sudden cutoff of
    // rendered text.
    start_line = base::sub_sat(start_line, 1_Z);
    end_line = base::add_sat(end_line, 1_Z);

    start_line = std::clamp(start_line, 0_Z, table.lineCount());
    end_line = std::clamp(end_line, 0_Z, table.lineCount());

    {
        PROFILE_BLOCK("TextViewWidget::renderText()");
        for (size_t line = start_line; line < end_line; ++line) {
            const auto& layout = layoutAt(line);

            Point coords = position - scroll_offset;
            // TODO: Using `metrics.line_height` causes a use-after-free error??
            //       The line with `line_layout_cache.getLineLayout()` is problematic.
            coords.y += line * main_line_height;

            int min_x = scroll_offset.x;
            int max_x = scroll_offset.x + size.width;

            text_renderer.renderLineLayout(layout, coords, min_x, max_x, kTextColor,
                                           TextRenderer::FontType::kMain);
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

    std::vector<SelectionRenderer::Selection> selections;
    for (size_t line = c1.line; line <= c2.line; ++line) {
        const auto& layout = layoutAt(line);
        int start = line == c1.line ? c1.x : 0;
        int end = line == c2.line ? c2.x : layout.width;
        if (end - start > 0) {
            selections.emplace_back(SelectionRenderer::Selection{
                .line = static_cast<int>(line),
                .start = start,
                .end = end,
            });
        }
    }
    selection_renderer.renderSelections(selections, position - scroll_offset);

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

    bool exclude_end;
    const auto& layout = layoutAt(new_line, exclude_end);
    end_caret.moveToX(layout, new_line, new_coords.x, exclude_end);
    // updateCaretX();
}

void TextViewWidget::updateMaxScroll() {
    const auto& glyph_cache = Renderer::instance().getGlyphCache();
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.getMetrics(glyph_cache.mainFontId());

    max_scroll_offset.x = line_layout_cache.maxWidth();
    max_scroll_offset.y = table.lineCount() * metrics.line_height;
}

size_t TextViewWidget::lineAtY(int y) {
    if (y < 0) {
        y = 0;
    }

    const auto& glyph_cache = Renderer::instance().getGlyphCache();
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.getMetrics(glyph_cache.mainFontId());

    size_t line = y / metrics.line_height;
    return std::clamp(line, 0_Z, base::sub_sat(table.lineCount(), 1_Z));
}

inline const font::LineLayout& TextViewWidget::layoutAt(size_t line) {
    bool unused;
    return layoutAt(line, unused);
}

inline const font::LineLayout& TextViewWidget::layoutAt(size_t line, bool& exclude_end) {
    std::string line_str = table.line(line);
    exclude_end = !line_str.empty() && line_str.back() == '\n';

    if (exclude_end) {
        line_str.back() = ' ';
    }

    return line_layout_cache.getLineLayout(line_str);
}

}
