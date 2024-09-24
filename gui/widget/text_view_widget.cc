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

    // TODO: See if we need to change this for proportional fonts.
    const auto& line_number_layout = line_layout_cache.getLineLayout("11");
    line_number_width = line_number_layout.width;
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

    if (to == MoveTo::kBOL || to == MoveTo::kHardBOL) {
        auto [line, _] = table.lineColumnAt(selection.end().index);

        bool exclude_end;
        const auto& layout = layoutAt(line, exclude_end);
        size_t new_col = Caret::columnAtX(layout, 0, exclude_end);
        selection.setIndex(table.indexAt(line, new_col), extend);
        // updateCaretX();
    }
    if (to == MoveTo::kEOL || to == MoveTo::kHardEOL) {
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

void TextViewWidget::rightDelete() {
    PROFILE_BLOCK("TextViewWidget::rightDelete()");

    if (selection.empty()) {
        auto [line, col] = table.lineColumnAt(selection.end().index);
        const auto& layout = layoutAt(line);

        size_t delta = Caret::moveToNextGlyph(layout, col);
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

void TextViewWidget::deleteWord(bool forward) {
    PROFILE_BLOCK("TextViewWidget::deleteWord()");

    if (selection.empty()) {
        auto [line, col] = table.lineColumnAt(selection.end().index);
        const auto& layout = layoutAt(line);

        size_t delta;
        if (forward) {
            delta = Caret::nextWordEnd(layout, col, table.line(line));
        } else {
            delta = Caret::prevWordStart(layout, col, table.line(line));
            selection.decrementIndex(delta, false);

            // Move to previous line if at beginning of line.
            if (delta == 0 && line > 0) {
                const auto& prev_layout = layoutAt(line - 1);
                size_t index = table.indexAt(line - 1, base::sub_sat(prev_layout.length, 1_Z));
                selection.setIndex(index, false);
            }
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
    size_t visible_lines = std::ceil(static_cast<double>(size.height) / main_line_height);

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
    Point new_coords = mouse_pos - textOffset();
    size_t new_line = lineAtY(new_coords.y);

    bool exclude_end;
    const auto& layout = layoutAt(new_line, exclude_end);
    size_t new_col = Caret::columnAtX(layout, new_coords.x, exclude_end);

    if (click_type == app::ClickType::kSingleClick) {
        bool extend = modifiers == app::ModifierKey::kShift;
        selection.setIndex(table.indexAt(new_line, new_col), extend);
    } else if (click_type == app::ClickType::kDoubleClick) {
        selection.setIndex(table.indexAt(new_line, new_col), false);
        size_t start_delta = Caret::prevWordStart(layout, new_col, table.line(new_line));
        size_t end_delta = Caret::nextWordEnd(layout, new_col, table.line(new_line));
        selection.start().index -= start_delta;
        selection.end().index += end_delta;
    }

    // updateCaretX();
}

void TextViewWidget::leftMouseDrag(const Point& mouse_pos,
                                   app::ModifierKey modifiers,
                                   app::ClickType click_type) {
    Point new_coords = mouse_pos - textOffset();
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

inline constexpr Point TextViewWidget::textOffset() const {
    Point text_offset = position - scroll_offset;
    text_offset.x += gutterWidth();
    return text_offset;
}

inline constexpr int TextViewWidget::gutterWidth() const {
    return kGutterLeftPadding + line_number_width + kGutterRightPadding;
}

void TextViewWidget::renderText(size_t start_line, size_t end_line, int main_line_height) {
    // Render two lines before start and one line after end. This ensures no sudden cutoff of
    // rendered text.
    start_line = base::sub_sat(start_line, 2_Z);
    end_line = base::add_sat(end_line, 1_Z);

    start_line = std::clamp(start_line, 0_Z, table.lineCount());
    end_line = std::clamp(end_line, 0_Z, table.lineCount());

    TextRenderer& text_renderer = Renderer::instance().getTextRenderer();
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();
    std::vector<base::SyntaxHighlighter::Highlight> highlights;
    {
        PROFILE_BLOCK("SyntaxHighlighter::getHighlights()");
        highlights = highlighter.getHighlights(start_line, end_line);
    }

    // TODO: Refactor code in draw() to only fetch caret [line, col] once.
    auto [selection_line, selection_col] = table.lineColumnAt(selection.end().index);

    PROFILE_BLOCK("TextViewWidget::renderText()");
    for (size_t line = start_line; line < end_line; ++line) {
        const auto& layout = layoutAt(line);

        Point coords = textOffset();
        coords.y += static_cast<int>(line) * main_line_height;
        coords.y -= main_line_height;

        coords.x += 3;  // Source Code Pro et al.
        // coords.x += 2;  // Chinese

        int min_x = scroll_offset.x;
        int max_x = scroll_offset.x + size.width;

        constexpr bool kHighlight = true;
        if constexpr (kHighlight) {
            text_renderer.renderLineLayout(layout, coords, min_x, max_x, kTextColor,
                                           TextRenderer::FontType::kMain, highlighter, highlights,
                                           line);
        } else {
            text_renderer.renderLineLayout(layout, coords, min_x, max_x, kTextColor,
                                           TextRenderer::FontType::kMain);
        }

        // Draw gutter.
        if (line == selection_line) {
            Point gutter_coords = position - scroll_offset;
            gutter_coords.y += static_cast<int>(line) * main_line_height;
            rect_renderer.addRect(gutter_coords, {gutterWidth(), main_line_height}, kGutterColor,
                                  RectRenderer::RectType::kBackground);
        }

        // Draw line numbers.
        Point line_number_coords = position - scroll_offset;
        line_number_coords.y += static_cast<int>(line) * main_line_height;
        line_number_coords.y -= main_line_height;
        line_number_coords.x += kGutterLeftPadding;

        std::string line_number_str = std::format("{}", line + 1);
        const auto& color = line == selection_line ? kSelectedLineNumberColor : kLineNumberColor;
        const auto& line_number_layout = line_layout_cache.getLineLayout(line_number_str);

        line_number_coords.x += line_number_width - line_number_layout.width;

        text_renderer.renderLineLayout(line_number_layout, line_number_coords, min_x, max_x, color,
                                       TextRenderer::FontType::kMain);
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
            // TODO: Formalize this. This matches Sublime Text's selections.
            if (start > 0) start += 2;
            end += 2;

            selections.emplace_back(SelectionRenderer::Selection{
                .line = static_cast<int>(line),
                .start = start,
                .end = end,
            });
        }
    }
    selection_renderer.renderSelections(selections, textOffset());
}

void TextViewWidget::renderScrollBars(int main_line_height, size_t visible_lines) {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();

    // Add vertical scroll bar.
    int line_height = main_line_height;
    int vbar_width = 15;
    int max_scrollbar_y = static_cast<int>(table.lineCount() + visible_lines) * line_height;
    double vbar_height_percent = static_cast<double>(size.height) / max_scrollbar_y;
    int vbar_height = static_cast<int>(size.height * vbar_height_percent);
    vbar_height = std::max(30, vbar_height);
    double vbar_percent = static_cast<double>(scroll_offset.y) / max_scroll_offset.y;
    Point vbar_coords{
        .x = size.width - vbar_width,
        .y = static_cast<int>(std::round((size.height - vbar_height) * vbar_percent)),
    };
    rect_renderer.addRect(vbar_coords + position, {vbar_width, vbar_height}, kScrollBarColor,
                          RectRenderer::RectType::kForeground, 5);

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
    caret_pos.y -= extra_padding;
    caret_pos += textOffset();

    rect_renderer.addRect(caret_pos, {caret_width, caret_height}, kCaretColor,
                          RectRenderer::RectType::kForeground);
}

}
