#include "base/numeric/literals.h"
#include "base/numeric/saturation_arithmetic.h"
#include "gui/renderer/renderer.h"
#include "text_view_widget.h"
#include <cmath>

// TODO: Debug use; remove this.
#include "util/profile_util.h"
#include <cassert>
#include <format>
#include <iostream>
#include <tree_sitter/api.h>

namespace gui {

TextViewWidget::TextViewWidget(std::string_view text) : table{text} {
    updateMaxScroll();

    highlighter.setJsonLanguage();
    highlighter.parse({&table, base::SyntaxHighlighter::read, TSInputEncodingUTF8});
}

void TextViewWidget::selectAll() {
    selection.setRange(0, table.length());

    // updateCaretX();
}

void TextViewWidget::move(MoveBy by, bool forward, bool extend) {
    PROFILE_BLOCK("TextViewWidget::move()");

    auto [line, col] = table.lineColumnAt(selection.end().index);
    const auto& layout = layoutAt(line);

    if (by == MoveBy::kCharacters && !forward) {
        if (!extend && !selection.empty()) {
            selection.collapse(Selection::Direction::kLeft);
            return;
        }

        size_t delta = Caret::moveToPrevGlyph(layout, col);
        selection.decrementIndex(delta, extend);

        // Move to previous line if at beginning of line.
        if (delta == 0 && line > 0) {
            const auto& prev_layout = layoutAt(line - 1);
            size_t index = table.indexAt(line - 1, base::sub_sat(prev_layout.length, 1_Z));
            selection.setIndex(index, extend);
        }
    }
    if (by == MoveBy::kCharacters && forward) {
        if (!extend && !selection.empty()) {
            selection.collapse(Selection::Direction::kRight);
            return;
        }

        size_t delta = Caret::moveToNextGlyph(layout, col);
        selection.incrementIndex(delta, extend);
    }
    if (by == MoveBy::kWords && !forward) {
        size_t delta = Caret::prevWordStart(layout, col, table.line(line));
        selection.decrementIndex(delta, extend);

        // Move to previous line if at beginning of line.
        if (delta == 0 && line > 0) {
            const auto& prev_layout = layoutAt(line - 1);
            size_t index = table.indexAt(line - 1, base::sub_sat(prev_layout.length, 1_Z));
            selection.setIndex(index, extend);
        }
    }
    if (by == MoveBy::kWords && forward) {
        size_t delta = Caret::nextWordEnd(layout, col, table.line(line));
        selection.incrementIndex(delta, extend);
    }
    // TODO: Find a clean way to combine vertical caret movement logic.
    if (by == MoveBy::kLines && !forward) {
        if (line > 0) {
            bool exclude_end;
            const auto& prev_layout = layoutAt(line - 1, exclude_end);
            int x = Caret::xAtColumn(layout, col, false);
            size_t new_col = Caret::columnAtX(prev_layout, x, exclude_end);
            size_t index = table.indexAt(line - 1, new_col);
            selection.setIndex(index, extend);
        }
    }
    if (by == MoveBy::kLines && forward) {
        if (line < base::sub_sat(table.lineCount(), 1_Z)) {
            bool exclude_end;
            const auto& prev_layout = layoutAt(line + 1, exclude_end);
            int x = Caret::xAtColumn(layout, col, false);
            size_t new_col = Caret::columnAtX(prev_layout, x, exclude_end);
            size_t index = table.indexAt(line + 1, new_col);
            selection.setIndex(index, extend);
        }
    }

    if (by == MoveBy::kCharacters) {
        // updateCaretX();
    }
}

void TextViewWidget::moveTo(MoveTo to, bool extend) {
    PROFILE_BLOCK("TextViewWidget::moveTo()");

    if (to == MoveTo::kHardBOL) {
        auto [line, _] = table.lineColumnAt(selection.end().index);

        bool exclude_end;
        const auto& layout = layoutAt(line, exclude_end);
        size_t new_col = Caret::columnAtX(layout, 0, exclude_end);
        selection.setIndex(table.indexAt(line, new_col), extend);
        // updateCaretX();
    }
    if (to == MoveTo::kHardEOL) {
        auto [line, _] = table.lineColumnAt(selection.end().index);

        bool exclude_end;
        const auto& layout = layoutAt(line, exclude_end);
        size_t new_col = Caret::columnAtX(layout, layout.width, exclude_end);
        selection.setIndex(table.indexAt(line, new_col), extend);
        // updateCaretX();
    }
    if (to == MoveTo::kBOF) {
        selection.setIndex(0, extend);
        // updateCaretX();
    }
    if (to == MoveTo::kEOF) {
        selection.setIndex(table.length(), extend);
        // updateCaretX();
    }
}

void TextViewWidget::insertText(std::string_view text) {
    PROFILE_BLOCK("TextViewWidget::insertText()");

    if (!selection.empty()) {
        leftDelete();
    }

    size_t i = selection.end().index;
    table.insert(i, text);
    selection.incrementIndex(text.length(), false);

    highlighter.edit(i, i, i + text.length());
    highlighter.parse({&table, base::SyntaxHighlighter::read, TSInputEncodingUTF8});

    // TODO: Do we update caret `max_x` too?

    updateMaxScroll();
}

void TextViewWidget::leftDelete() {
    PROFILE_BLOCK("TextViewWidget::leftDelete()");

    if (selection.empty()) {
        auto [line, col] = table.lineColumnAt(selection.end().index);
        const auto& layout = layoutAt(line);

        size_t delta = Caret::moveToPrevGlyph(layout, col);
        selection.decrementIndex(delta, false);

        // Delete newline if at beginning of line.
        if (delta == 0 && line > 0) {
            selection.decrementIndex(1_Z, false);
            delta = 1;
        }

        size_t i = selection.end().index;
        table.erase(i, delta);

        highlighter.edit(i, i + delta, i);
        highlighter.parse({&table, base::SyntaxHighlighter::read, TSInputEncodingUTF8});
    } else {
        auto [start, end] = selection.range();
        table.erase(start, end - start);
        selection.collapse(Selection::Direction::kLeft);

        highlighter.edit(start, end, start);
        highlighter.parse({&table, base::SyntaxHighlighter::read, TSInputEncodingUTF8});
    }
}

std::string TextViewWidget::getSelectionText() {
    auto [start, end] = selection.range();
    return table.substr(start, end - start);
}

void TextViewWidget::draw() {
    const auto& glyph_cache = Renderer::instance().getGlyphCache();
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.getMetrics(glyph_cache.mainFontId());

    // Calculate start and end lines.
    int main_line_height = metrics.line_height;
    size_t visible_lines = std::ceil(static_cast<float>(size.height) / main_line_height);

    size_t start_line = scroll_offset.y / main_line_height;
    size_t end_line = start_line + visible_lines;

    renderText(start_line, end_line, main_line_height);
    renderSelections(start_line, end_line);
    renderScrollBars(main_line_height, visible_lines);
    renderCaret(main_line_height);
}

void TextViewWidget::leftMouseDown(const Point& mouse_pos,
                                   app::ModifierKey modifiers,
                                   app::ClickType click_type) {
    Point new_coords = mouse_pos - position + scroll_offset;
    size_t new_line = lineAtY(new_coords.y);

    bool exclude_end;
    const auto& layout = layoutAt(new_line, exclude_end);
    size_t new_col = Caret::columnAtX(layout, new_coords.x, exclude_end);

    bool extend = modifiers == app::ModifierKey::kShift;
    selection.setIndex(table.indexAt(new_line, new_col), extend);

    // updateCaretX();
}

void TextViewWidget::leftMouseDrag(const Point& mouse_pos,
                                   app::ModifierKey modifiers,
                                   app::ClickType click_type) {
    Point new_coords = mouse_pos - position + scroll_offset;
    size_t new_line = lineAtY(new_coords.y);

    bool exclude_end;
    const auto& layout = layoutAt(new_line, exclude_end);
    size_t new_col = Caret::columnAtX(layout, new_coords.x, exclude_end);
    selection.setIndex(table.indexAt(new_line, new_col), true);

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

void TextViewWidget::renderText(size_t start_line, size_t end_line, int main_line_height) {
    // Render two lines before start and one line after end. This ensures no sudden cutoff of
    // rendered text.
    start_line = base::sub_sat(start_line, 2_Z);
    end_line = base::add_sat(end_line, 1_Z);

    start_line = std::clamp(start_line, 0_Z, table.lineCount());
    end_line = std::clamp(end_line, 0_Z, table.lineCount());

    TextRenderer& text_renderer = Renderer::instance().getTextRenderer();
    std::vector<base::SyntaxHighlighter::Highlight> highlights;
    {
        PROFILE_BLOCK("SyntaxHighlighter::getHighlights()");
        highlights = highlighter.getHighlights(start_line, end_line);

        // std::cerr << "Printing all highlights:\n";
        // for (const auto& h : highlights) {
        //     std::cerr << h.start << ' ' << h.end
        //               << std::format(" , capture_index = {}\n", h.capture_index);
        // }
        // std::cerr << "Done\n";
    }
    {
        PROFILE_BLOCK("TextViewWidget::renderText()");
        for (size_t line = start_line; line < end_line; ++line) {
            const auto& layout = layoutAt(line);

            Point coords = position - scroll_offset;
            // TODO: Using `metrics.line_height` causes a use-after-free error??
            //       The line with `line_layout_cache.getLineLayout()` is problematic.
            coords.y += line * main_line_height;

            // TODO: These changes are optimal to match Sublime Text's layout. Formalize this.
            coords.y -= main_line_height;
            // TODO: Seems like Sublime Text adds something to the left edge no matter what.
            // We could formalize this as padding, but we need selection to be flush...
            // coords.x += 3;  // Source Code Pro et al.
            // coords.x += 2;  // Chinese

            int min_x = scroll_offset.x;
            int max_x = scroll_offset.x + size.width;

            text_renderer.renderLineLayout(layout, coords, min_x, max_x, kTextColor,
                                           TextRenderer::FontType::kMain, highlighter, highlights,
                                           line);
        }
    }
    constexpr bool kDebugAtlas = false;
    if constexpr (kDebugAtlas) {
        text_renderer.renderAtlasPages(position);
    }
}

void TextViewWidget::renderSelections(size_t start_line, size_t end_line) {
    SelectionRenderer& selection_renderer = Renderer::instance().getSelectionRenderer();
    auto [start, end] = selection.range();
    auto [c1_line, c1_col] = table.lineColumnAt(start);
    auto [c2_line, c2_col] = table.lineColumnAt(end);

    const auto& c1_layout = layoutAt(c1_line);
    const auto& c2_layout = layoutAt(c2_line);
    int c1_x = Caret::xAtColumn(c1_layout, c1_col);
    int c2_x = Caret::xAtColumn(c2_layout, c2_col);

    // Don't render off-screen selections.
    if (c1_line < start_line) c1_line = start_line;
    if (c2_line > end_line) c2_line = end_line;

    std::vector<SelectionRenderer::Selection> selections;
    for (size_t line = c1_line; line <= c2_line; ++line) {
        const auto& layout = layoutAt(line);
        int start = line == c1_line ? c1_x : 0;
        int end = line == c2_line ? c2_x : layout.width;

        if (end - start > 0) {
            selections.emplace_back(SelectionRenderer::Selection{
                .line = static_cast<int>(line),
                .start = start,
                .end = end,
            });
        }
    }
    selection_renderer.renderSelections(selections, position - scroll_offset);
}

void TextViewWidget::renderScrollBars(int main_line_height, size_t visible_lines) {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();

    // Add vertical scroll bar.
    int line_count = table.lineCount();
    int line_height = main_line_height;
    int vbar_width = 15;
    int max_scrollbar_y = (line_count + visible_lines) * line_height;
    int vbar_height = size.height * (static_cast<float>(size.height) / max_scrollbar_y);
    vbar_height = std::max(30, vbar_height);
    float vbar_percent = static_cast<float>(scroll_offset.y) / max_scroll_offset.y;
    Point vbar_coords{
        .x = size.width - vbar_width,
        .y = static_cast<int>(std::round((size.height - vbar_height) * vbar_percent)),
    };
    rect_renderer.addRect(vbar_coords + position, {vbar_width, vbar_height}, kScrollBarColor, 5);

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
}

void TextViewWidget::renderCaret(int main_line_height) {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();

    int caret_width = 4;
    int extra_padding = 8;
    int caret_height = main_line_height + extra_padding * 2;

    auto [line, col] = table.lineColumnAt(selection.end().index);
    bool exclude_end;
    const auto& layout = layoutAt(line, exclude_end);
    int end_caret_x = Caret::xAtColumn(layout, col, exclude_end);

    Point caret_pos{
        .x = end_caret_x,
        .y = static_cast<int>(line) * main_line_height,
    };
    caret_pos += position;
    caret_pos -= scroll_offset;
    caret_pos.x -= caret_width / 2;
    caret_pos.y -= extra_padding;
    rect_renderer.addRect(caret_pos, {caret_width, caret_height}, kCaretColor);
}

}
