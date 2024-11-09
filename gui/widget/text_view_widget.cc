#include "base/numeric/literals.h"
#include "base/numeric/saturation_arithmetic.h"
#include "gui/renderer/renderer.h"
#include "text_view_widget.h"
#include <cmath>

// TODO: Debug use; remove this.
#include "util/profile_util.h"
#include "util/std_print.h"
#include <cassert>
#include <tree_sitter/api.h>

namespace gui {

TextViewWidget::TextViewWidget(std::string_view text)
    : tree{text}, line_layout_cache{Renderer::instance().getGlyphCache().mainFontId()} {
    updateMaxScroll();

#ifdef ENABLE_HIGHLIGHTING
    highlighter.setJsonLanguage();
    highlighter.parse({&table, base::SyntaxHighlighter::read, TSInputEncodingUTF8});
#endif
}

void TextViewWidget::selectAll() {
    selection.setRange(0, tree.length());

    // updateCaretX();
}

void TextViewWidget::move(MoveBy by, bool forward, bool extend) {
    PROFILE_BLOCK("TextViewWidget::move()");

    auto [line, col] = tree.line_column_at(selection.end().index);
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
            size_t index = tree.offset_at(line - 1, base::sub_sat(prev_layout.length, 1_Z));
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
        size_t delta = Caret::prevWordStart(layout, col, tree.get_line_content_with_newline(line));
        selection.decrementIndex(delta, extend);

        // Move to previous line if at beginning of line.
        if (delta == 0 && line > 0) {
            const auto& prev_layout = layoutAt(line - 1);
            size_t index = tree.offset_at(line - 1, base::sub_sat(prev_layout.length, 1_Z));
            selection.setIndex(index, extend);
        }
    }
    if (by == MoveBy::kWords && forward) {
        size_t delta = Caret::nextWordEnd(layout, col, tree.get_line_content_with_newline(line));
        selection.incrementIndex(delta, extend);
    }
    // TODO: Find a clean way to combine vertical caret movement logic.
    if (by == MoveBy::kLines && !forward) {
        if (line > 0) {
            bool exclude_end;
            const auto& prev_layout = layoutAt(line - 1, exclude_end);
            int x = Caret::xAtColumn(layout, col, false);
            size_t new_col = Caret::columnAtX(prev_layout, x, exclude_end);
            size_t index = tree.offset_at(line - 1, new_col);
            selection.setIndex(index, extend);
        }
    }
    if (by == MoveBy::kLines && forward) {
        if (line < tree.line_count() - 1) {
            bool exclude_end;
            const auto& prev_layout = layoutAt(line + 1, exclude_end);
            int x = Caret::xAtColumn(layout, col, false);
            size_t new_col = Caret::columnAtX(prev_layout, x, exclude_end);
            size_t index = tree.offset_at(line + 1, new_col);
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
        size_t line = tree.line_at(selection.end().index);

        bool exclude_end;
        const auto& layout = layoutAt(line, exclude_end);
        size_t new_col = Caret::columnAtX(layout, 0, exclude_end);
        selection.setIndex(tree.offset_at(line, new_col), extend);
        // updateCaretX();
    }
    if (to == MoveTo::kEOL || to == MoveTo::kHardEOL) {
        size_t line = tree.line_at(selection.end().index);

        bool exclude_end;
        const auto& layout = layoutAt(line, exclude_end);
        size_t new_col = Caret::columnAtX(layout, layout.width, exclude_end);
        selection.setIndex(tree.offset_at(line, new_col), extend);
        // updateCaretX();
    }
    if (to == MoveTo::kBOF) {
        selection.setIndex(0, extend);
        // updateCaretX();
    }
    if (to == MoveTo::kEOF) {
        selection.setIndex(tree.length(), extend);
        // updateCaretX();
    }
}

void TextViewWidget::insertText(std::string_view text) {
    PROFILE_BLOCK("TextViewWidget::insertText()");

    if (!selection.empty()) {
        leftDelete();
    }

    size_t i = selection.end().index;
    tree.insert(i, text);
    selection.incrementIndex(text.length(), false);

#ifdef ENABLE_HIGHLIGHTING
    highlighter.edit(i, i, i + text.length());
    highlighter.parse({&table, base::SyntaxHighlighter::read, TSInputEncodingUTF8});
#endif

    // TODO: Do we update caret `max_x` too?

    updateMaxScroll();
}

void TextViewWidget::leftDelete() {
    PROFILE_BLOCK("TextViewWidget::leftDelete()");

    if (selection.empty()) {
        auto [line, col] = tree.line_column_at(selection.end().index);
        const auto& layout = layoutAt(line);

        size_t delta = Caret::moveToPrevGlyph(layout, col);
        selection.decrementIndex(delta, false);

        // Delete newline if at beginning of line.
        if (delta == 0 && line > 0) {
            selection.decrementIndex(1_Z, false);
            delta = 1;
        }

        size_t i = selection.end().index;
        tree.erase(i, delta);

#ifdef ENABLE_HIGHLIGHTING
        highlighter.edit(i, i + delta, i);
        highlighter.parse({&table, base::SyntaxHighlighter::read, TSInputEncodingUTF8});
#endif
    } else {
        auto [start, end] = selection.range();
        tree.erase(start, end - start);
        selection.collapse(Selection::Direction::kLeft);

#ifdef ENABLE_HIGHLIGHTING
        highlighter.edit(start, end, start);
        highlighter.parse({&table, base::SyntaxHighlighter::read, TSInputEncodingUTF8});
#endif
    }

    updateMaxScroll();
}

void TextViewWidget::rightDelete() {
    PROFILE_BLOCK("TextViewWidget::rightDelete()");

    if (selection.empty()) {
        auto [line, col] = tree.line_column_at(selection.end().index);
        const auto& layout = layoutAt(line);

        size_t delta = Caret::moveToNextGlyph(layout, col);
        size_t i = selection.end().index;
        tree.erase(i, delta);

#ifdef ENABLE_HIGHLIGHTING
        highlighter.edit(i, i + delta, i);
        highlighter.parse({&table, base::SyntaxHighlighter::read, TSInputEncodingUTF8});
#endif
    } else {
        auto [start, end] = selection.range();
        tree.erase(start, end - start);
        selection.collapse(Selection::Direction::kLeft);

#ifdef ENABLE_HIGHLIGHTING
        highlighter.edit(start, end, start);
        highlighter.parse({&table, base::SyntaxHighlighter::read, TSInputEncodingUTF8});
#endif
    }

    updateMaxScroll();
}

void TextViewWidget::deleteWord(bool forward) {
    PROFILE_BLOCK("TextViewWidget::deleteWord()");

    if (selection.empty()) {
        auto [line, col] = tree.line_column_at(selection.end().index);
        const auto& layout = layoutAt(line);

        size_t delta;
        if (forward) {
            delta = Caret::nextWordEnd(layout, col, tree.get_line_content_with_newline(line));
        } else {
            delta = Caret::prevWordStart(layout, col, tree.get_line_content_with_newline(line));
            selection.decrementIndex(delta, false);

            // Delete newline if at beginning of line.
            if (delta == 0 && line > 0) {
                selection.decrementIndex(1_Z, false);
                delta = 1;
            }
        }

        size_t i = selection.end().index;
        tree.erase(i, delta);

#ifdef ENABLE_HIGHLIGHTING
        highlighter.edit(i, i + delta, i);
        highlighter.parse({&table, base::SyntaxHighlighter::read, TSInputEncodingUTF8});
#endif
    } else {
        auto [start, end] = selection.range();
        tree.erase(start, end - start);
        selection.collapse(Selection::Direction::kLeft);

#ifdef ENABLE_HIGHLIGHTING
        highlighter.edit(start, end, start);
        highlighter.parse({&table, base::SyntaxHighlighter::read, TSInputEncodingUTF8});
#endif
    }

    updateMaxScroll();
}

std::string TextViewWidget::getSelectionText() {
    auto [start, end] = selection.range();
    return tree.substr(start, end - start);
}

void TextViewWidget::undo() {
    tree.try_undo();
}

void TextViewWidget::redo() {
    tree.try_redo();
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
    renderScrollBars(main_line_height);
    renderCaret(main_line_height);
}

void TextViewWidget::leftMouseDown(const app::Point& mouse_pos,
                                   app::ModifierKey modifiers,
                                   app::ClickType click_type) {
    app::Point new_coords = mouse_pos - textOffset();
    size_t new_line = lineAtY(new_coords.y);

    bool exclude_end;
    const auto& layout = layoutAt(new_line, exclude_end);
    size_t new_col = Caret::columnAtX(layout, new_coords.x, exclude_end);

    if (click_type == app::ClickType::kSingleClick) {
        bool extend = modifiers == app::ModifierKey::kShift;
        selection.setIndex(tree.offset_at(new_line, new_col), extend);
    }
    // TODO: Implement double clicks.
    // else if (click_type == app::ClickType::kDoubleClick) {
    //     selection.setIndex(tree.offset_at(new_line, new_col), false);
    //     size_t start_delta =
    //         Caret::prevWordStart(layout, new_col, tree.get_line_content_with_newline(new_line));
    //     size_t end_delta =
    //         Caret::nextWordEnd(layout, new_col, tree.get_line_content_with_newline(new_line));
    //     selection.start().index -= start_delta;
    //     selection.end().index += end_delta;
    // }

    // updateCaretX();
}

void TextViewWidget::leftMouseDrag(const app::Point& mouse_pos,
                                   app::ModifierKey modifiers,
                                   app::ClickType click_type) {
    app::Point new_coords = mouse_pos - textOffset();
    size_t new_line = lineAtY(new_coords.y);

    bool exclude_end;
    const auto& layout = layoutAt(new_line, exclude_end);
    size_t new_col = Caret::columnAtX(layout, new_coords.x, exclude_end);
    selection.setIndex(tree.offset_at(new_line, new_col), true);

    // updateCaretX();
}

void TextViewWidget::updateMaxScroll() {
    const auto& glyph_cache = Renderer::instance().getGlyphCache();
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.getMetrics(glyph_cache.mainFontId());

    max_scroll_offset.x = line_layout_cache.maxWidth();
    max_scroll_offset.y = tree.line_count() * metrics.line_height;
}

size_t TextViewWidget::lineAtY(int y) const {
    if (y < 0) {
        y = 0;
    }

    const auto& glyph_cache = Renderer::instance().getGlyphCache();
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.getMetrics(glyph_cache.mainFontId());

    size_t line = y / metrics.line_height;
    return std::clamp(line, 0_Z, tree.line_count() - 1);
}

inline const font::LineLayout& TextViewWidget::layoutAt(size_t line) {
    bool unused;
    return layoutAt(line, unused);
}

inline const font::LineLayout& TextViewWidget::layoutAt(size_t line, bool& exclude_end) {
    std::string line_str = tree.get_line_content_with_newline(line);
    exclude_end = !line_str.empty() && line_str.back() == '\n';

    if (exclude_end) {
        line_str.back() = ' ';
    }

    return line_layout_cache[line_str];
}

inline constexpr app::Point TextViewWidget::textOffset() {
    app::Point text_offset = position - scroll_offset;
    text_offset.x += gutterWidth();
    return text_offset;
}

inline constexpr int TextViewWidget::gutterWidth() {
    return kGutterLeftPadding + lineNumberWidth() + kGutterRightPadding;
}

inline constexpr int TextViewWidget::lineNumberWidth() {
    int digit_width = line_layout_cache["0"].width;
    int log = std::log10(tree.line_count());
    return digit_width * std::max(log + 1, 2);
}

void TextViewWidget::renderText(size_t start_line, size_t end_line, int main_line_height) {
    // Render two lines before start and one line after end. This ensures no sudden cutoff of
    // rendered text.
    start_line = base::sub_sat(start_line, 2_Z);
    end_line = base::add_sat(end_line, 1_Z);

    start_line = std::clamp(start_line, 0_Z, tree.line_count());
    end_line = std::clamp(end_line, 0_Z, tree.line_count());

    TextRenderer& text_renderer = Renderer::instance().getTextRenderer();
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();

#ifdef ENABLE_HIGHLIGHTING
    std::vector<base::SyntaxHighlighter::Highlight> highlights;
    {
        PROFILE_BLOCK("SyntaxHighlighter::getHighlights()");
        highlights = highlighter.getHighlights(start_line, end_line);
    }
#endif

    // TODO: Refactor code in draw() to only fetch caret [line, col] once.
    size_t selection_line = tree.line_at(selection.end().index);

    PROFILE_BLOCK("TextViewWidget::renderText()");

    // long long total_layout_duration = 0;
    // long long total_text_render_duration = 0;

    for (size_t line = start_line; line < end_line; ++line) {
        auto t1 = std::chrono::high_resolution_clock::now();
        const auto& layout = layoutAt(line);
        auto t2 = std::chrono::high_resolution_clock::now();
        // long long duration =
        //     std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        // total_layout_duration += duration;

        app::Point coords = textOffset();
        coords.y += static_cast<int>(line) * main_line_height;
        coords.x += kCaretWidth / 2;  // Match Sublime Text.

        int min_x = scroll_offset.x;
        int max_x = scroll_offset.x + size.width;

#ifdef ENABLE_HIGHLIGHTING
        std::stack<base::SyntaxHighlighter::Highlight> stk;
        auto it = highlights.begin();
        const auto highlight_callback = [&](size_t col) {
            TSPoint p{
                .row = static_cast<uint32_t>(line),
                .column = static_cast<uint32_t>(col),
            };

            // Use stack to parse highlights.
            while (it != highlights.end() && p >= (*it).end) {
                ++it;
            }
            while (it != highlights.end() && (*it).containsPoint(p)) {
                // If multiple ranges are equal, prefer the one that comes first.
                if (stk.empty() || stk.top() != *it) {
                    stk.push(*it);
                }
                ++it;
            }
            while (!stk.empty() && p >= stk.top().end) {
                stk.pop();
            }

            size_t capture_index = 0;
            if (!stk.empty() && stk.top().containsPoint(p)) {
                capture_index = stk.top().capture_index;

                // TODO: Use unified Rgb struct.
                const auto& highlight_color = highlighter.getColor(capture_index);
                const Rgb rgb{
                    .r = highlight_color.r, .g = highlight_color.g, .b = highlight_color.b};
                return rgb;
            } else {
                return kTextColor;
            }
        };
#endif

        t1 = std::chrono::high_resolution_clock::now();
#ifdef ENABLE_HIGHLIGHTING
        text_renderer.renderLineLayout(layout, coords, TextRenderer::TextLayer::kBackground,
                                       highlight_callback, min_x, max_x);
#else
        text_renderer.renderLineLayout(
            layout, coords, TextRenderer::TextLayer::kBackground,
            [](size_t) { return kTextColor; }, min_x, max_x);
#endif
        t2 = std::chrono::high_resolution_clock::now();
        // duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        // total_text_render_duration += duration;

        // Draw gutter.
        if (line == selection_line) {
            app::Point gutter_coords = position;
            gutter_coords.y -= scroll_offset.y;
            gutter_coords.y += static_cast<int>(line) * main_line_height;
            rect_renderer.addRect(gutter_coords, {gutterWidth(), main_line_height}, kGutterColor,
                                  RectRenderer::RectLayer::kBackground);
        }

        // Draw line numbers.
        app::Point line_number_coords = position;
        line_number_coords.y -= scroll_offset.y;
        line_number_coords.x += kGutterLeftPadding;
        line_number_coords.y += static_cast<int>(line) * main_line_height;

        std::string line_number_str = std::format("{}", line + 1);
        const auto& line_number_layout = line_layout_cache[line_number_str];
        line_number_coords.x += lineNumberWidth() - line_number_layout.width;

        const auto line_number_highlight_callback = [&line, &selection_line](size_t) {
            return line == selection_line ? kSelectedLineNumberColor : kLineNumberColor;
        };
        text_renderer.renderLineLayout(line_number_layout, line_number_coords,
                                       TextRenderer::TextLayer::kBackground,
                                       line_number_highlight_callback);
    }

    // std::println("Total layoutAt() time: {}", total_layout_duration);
    // std::println("Total TextRender time: {}", total_text_render_duration);

    constexpr bool kDebugAtlas = false;
    if constexpr (kDebugAtlas) {
        text_renderer.renderAtlasPages(position);
    }
}

void TextViewWidget::renderSelections(size_t start_line, size_t end_line) {
    SelectionRenderer& selection_renderer = Renderer::instance().getSelectionRenderer();
    auto [start, end] = selection.range();
    auto [c1_line, c1_col] = tree.line_column_at(start);
    auto [c2_line, c2_col] = tree.line_column_at(end);

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
            // Match Sublime Text.
            if (start > 0) start += kCaretWidth / 2;
            end += kCaretWidth / 2;

            selections.emplace_back(SelectionRenderer::Selection{
                .line = static_cast<int>(line),
                .start = start,
                .end = end,
            });
        }
    }
    selection_renderer.renderSelections(selections, textOffset());
}

void TextViewWidget::renderScrollBars(int main_line_height) {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();

    // Add vertical scroll bar.
    int vbar_width = 15;
    double max_scrollbar_y = size.height + tree.line_count() * main_line_height;
    double vbar_height_percent = static_cast<double>(size.height) / max_scrollbar_y;
    int vbar_height = static_cast<int>(size.height * vbar_height_percent);
    vbar_height = std::max(30, vbar_height);
    double vbar_percent = static_cast<double>(scroll_offset.y) / max_scroll_offset.y;
    app::Point vbar_coords{
        .x = size.width - vbar_width,
        .y = static_cast<int>(std::round((size.height - vbar_height) * vbar_percent)),
    };
    rect_renderer.addRect(vbar_coords + position, {vbar_width, vbar_height}, kScrollBarColor,
                          RectRenderer::RectLayer::kForeground, 5);

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

    int extra_padding = 8;
    int caret_height = main_line_height + extra_padding * 2;

    auto [line, col] = tree.line_column_at(selection.end().index);
    bool exclude_end;
    const auto& layout = layoutAt(line, exclude_end);
    int end_caret_x = Caret::xAtColumn(layout, col, exclude_end);

    app::Point caret_pos{
        .x = end_caret_x,
        .y = static_cast<int>(line) * main_line_height,
    };
    caret_pos.y -= extra_padding;
    caret_pos += textOffset();

    rect_renderer.addRect(caret_pos, {kCaretWidth, caret_height}, kCaretColor,
                          RectRenderer::RectLayer::kForeground);
}

}  // namespace gui
