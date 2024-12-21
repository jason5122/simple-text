#include "text_view_widget.h"

#include "base/numeric/literals.h"
#include "base/numeric/saturation_arithmetic.h"
#include "gui/renderer/renderer.h"

#include <cmath>

// TODO: Debug use; remove this.
#include "util/profile_util.h"
#include "util/std_print.h"
#include <cassert>

namespace gui {

TextViewWidget::TextViewWidget(std::string_view text, size_t font_id)
    : font_id(font_id), tree(text) {
    updateMaxScroll();

#ifdef ENABLE_HIGHLIGHTING
    auto& highlighter = highlight::Highlighter::instance();
    auto& language = highlighter.getLanguage("cpp");
    PROFILE_BLOCK("TextViewWidget: highlighter.parse()");
    parse_tree.parse(tree, language);
#endif
}

void TextViewWidget::selectAll() {
    selection.setRange(0, tree.length());
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
        selection.decrement(delta, extend);

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
        selection.increment(delta, extend);
    }
    if (by == MoveBy::kLines) {
        size_t new_line = forward ? line + 1 : line - 1;
        if (0 <= new_line && new_line < tree.line_count()) {
            int x = Caret::xAtColumn(layout, col);
            size_t new_col = Caret::columnAtX(layoutAt(new_line), x);
            size_t index = tree.offset_at(new_line, new_col);
            selection.setIndex(index, extend);
        }
    }
    if (by == MoveBy::kWords) {
        if (forward) {
            selection.end().index = Caret::nextWordEnd(tree, selection.end().index);
        } else {
            selection.end().index = Caret::prevWordStart(tree, selection.end().index);
        }
        if (!extend) {
            selection.start() = selection.end();
        }
    }

    if (by == MoveBy::kCharacters) {}
}

void TextViewWidget::moveTo(MoveTo to, bool extend) {
    PROFILE_BLOCK("TextViewWidget::moveTo()");

    if (to == MoveTo::kBOL || to == MoveTo::kHardBOL) {
        size_t line = tree.line_at(selection.end().index);

        const auto& layout = layoutAt(line);
        size_t new_col = Caret::columnAtX(layout, 0);
        selection.setIndex(tree.offset_at(line, new_col), extend);
    }
    if (to == MoveTo::kEOL || to == MoveTo::kHardEOL) {
        size_t line = tree.line_at(selection.end().index);

        const auto& layout = layoutAt(line);
        size_t new_col = Caret::columnAtX(layout, layout.width);
        selection.setIndex(tree.offset_at(line, new_col), extend);
    }
    if (to == MoveTo::kBOF) {
        selection.setIndex(0, extend);
    }
    if (to == MoveTo::kEOF) {
        selection.setIndex(tree.length(), extend);
    }
}

void TextViewWidget::insertText(std::string_view text) {
    if (!selection.empty()) {
        leftDelete();
    }

    size_t i = selection.end().index;
    tree.insert(i, text);
    selection.increment(text.length(), false);

#ifdef ENABLE_HIGHLIGHTING
    PROFILE_BLOCK("TextViewWidget::insertText() edit + parse");
    auto& highlighter = highlight::Highlighter::instance();
    auto& language = highlighter.getLanguage("cpp");
    parse_tree.edit(i, i, i + text.length());
    parse_tree.parse(tree, language);
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
        selection.decrement(delta, false);

        // Delete newline if at beginning of line.
        if (delta == 0 && line > 0) {
            selection.decrement(1_Z, false);
            delta = 1;
        }

        size_t i = selection.end().index;
        tree.erase(i, delta);

#ifdef ENABLE_HIGHLIGHTING
        auto& highlighter = highlight::Highlighter::instance();
        auto& language = highlighter.getLanguage("cpp");
        parse_tree.edit(i, i + delta, i);
        parse_tree.parse(tree, language);
#endif
    } else {
        auto [start, end] = selection.range();
        tree.erase(start, end - start);
        selection.collapse(Selection::Direction::kLeft);

#ifdef ENABLE_HIGHLIGHTING
        auto& highlighter = highlight::Highlighter::instance();
        auto& language = highlighter.getLanguage("cpp");
        parse_tree.edit(start, end, start);
        parse_tree.parse(tree, language);
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
        auto& highlighter = highlight::Highlighter::instance();
        auto& language = highlighter.getLanguage("cpp");
        parse_tree.edit(i, i + delta, i);
        parse_tree.parse(tree, language);
#endif
    } else {
        auto [start, end] = selection.range();
        tree.erase(start, end - start);
        selection.collapse(Selection::Direction::kLeft);

#ifdef ENABLE_HIGHLIGHTING
        auto& highlighter = highlight::Highlighter::instance();
        auto& language = highlighter.getLanguage("cpp");
        parse_tree.edit(start, end, start);
        parse_tree.parse(tree, language);
#endif
    }

    updateMaxScroll();
}

// TODO: Make this delete newlines without going past them into the previous line.
void TextViewWidget::deleteWord(bool forward) {
    PROFILE_BLOCK("TextViewWidget::deleteWord()");

    if (selection.empty()) {
        size_t prev_offset = selection.end().index;
        size_t offset, delta;
        if (forward) {
            offset = Caret::nextWordEnd(tree, prev_offset);
            delta = offset - prev_offset;
            tree.erase(prev_offset, delta);

            // TODO: Clean up selection/caret code.
            // TODO: After clean up, move this out of TextViewWidget.
            selection.end().index = prev_offset;
            selection.start() = selection.end();
        } else {
            offset = Caret::prevWordStart(tree, prev_offset);
            delta = prev_offset - offset;
            tree.erase(offset, delta);

            // TODO: Clean up selection/caret code.
            // TODO: After clean up, move this out of TextViewWidget.
            selection.end().index = offset;
            selection.start() = selection.end();
        }

        // #ifdef ENABLE_HIGHLIGHTING
        //         auto& highlighter = highlight::Highlighter::instance();
        //         auto& language = highlighter.getLanguage("cpp");
        //         parse_tree.edit(i, i + delta, i);
        //         parse_tree.parse(tree, language);
        // #endif
    } else {
        auto [start, end] = selection.range();
        tree.erase(start, end - start);
        selection.collapse(Selection::Direction::kLeft);

        // #ifdef ENABLE_HIGHLIGHTING
        //         auto& highlighter = highlight::Highlighter::instance();
        //         auto& language = highlighter.getLanguage("cpp");
        //         parse_tree.edit(start, end, start);
        //         parse_tree.parse(tree, language);
        // #endif
    }
}

std::string TextViewWidget::getSelectionText() {
    auto [start, end] = selection.range();
    return tree.substr(start, end - start);
}

void TextViewWidget::undo() {
    tree.undo();
}

void TextViewWidget::redo() {
    tree.redo();
}

void TextViewWidget::draw() {
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);

    // Calculate start and end lines.
    int main_line_height = metrics.line_height;
    size_t visible_lines = std::ceil(static_cast<double>(size.height) / main_line_height);

    size_t start_line = scroll_offset.y / main_line_height;
    size_t end_line = start_line + visible_lines;

    renderText(main_line_height, start_line, end_line);
    renderSelections(main_line_height, start_line, end_line);
    renderScrollBars(main_line_height);
    renderCaret(main_line_height);
}

void TextViewWidget::leftMouseDown(const app::Point& mouse_pos,
                                   app::ModifierKey modifiers,
                                   app::ClickType click_type) {
    app::Point new_coords = mouse_pos - textOffset();
    size_t line = lineAtY(new_coords.y);
    size_t col = Caret::columnAtX(layoutAt(line), new_coords.x);
    size_t offset = tree.offset_at(line, col);

    if (click_type == app::ClickType::kSingleClick) {
        bool extend = modifiers == app::ModifierKey::kShift;
        selection.setIndex(offset, extend);
    } else if (click_type == app::ClickType::kDoubleClick) {
        // TODO: Refine double click implementation.
        size_t left = Caret::prevWordStart(tree, offset);
        size_t right = Caret::nextWordEnd(tree, offset);
        selection.setRange(left, right);
    } else if (click_type == app::ClickType::kTripleClick) {
        auto [left, right] = tree.get_line_range_with_newline(line);
        selection.setRange(left, right);
    }
}

void TextViewWidget::leftMouseDrag(const app::Point& mouse_pos,
                                   app::ModifierKey modifiers,
                                   app::ClickType click_type) {
    app::Point new_coords = mouse_pos - textOffset();
    size_t new_line = lineAtY(new_coords.y);
    size_t new_col = Caret::columnAtX(layoutAt(new_line), new_coords.x);
    selection.setIndex(tree.offset_at(new_line, new_col), true);
}

void TextViewWidget::updateMaxScroll() {
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);

    // TODO: Figure out how to update max width.
    max_scroll_offset.x = 0;
    max_scroll_offset.y = tree.line_count() * metrics.line_height;
}

size_t TextViewWidget::lineAtY(int y) const {
    if (y < 0) {
        y = 0;
    }

    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);

    size_t line = y / metrics.line_height;
    return std::clamp(line, 0_Z, tree.line_count() - 1);
}

inline const font::LineLayout& TextViewWidget::layoutAt(size_t line) {
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    std::string line_str = tree.get_line_content_for_layout_use(line);
    return line_layout_cache.get(font_id, line_str);
}

inline constexpr app::Point TextViewWidget::textOffset() {
    app::Point text_offset = position - scroll_offset;
    text_offset.x += gutterWidth();
    return text_offset;
}

inline constexpr int TextViewWidget::gutterWidth() {
    return kGutterLeftPadding + lineNumberWidth() + kGutterRightPadding;
}

inline int TextViewWidget::lineNumberWidth() {
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    int digit_width = line_layout_cache.get(font_id, "0").width;
    int log = std::log10(tree.line_count());
    return digit_width * std::max(log + 1, 2);
}

void TextViewWidget::renderText(int main_line_height, size_t start_line, size_t end_line) {
    // Render two lines before start and one line after end. This ensures no sudden cutoff of
    // rendered text.
    start_line = base::sub_sat(start_line, 2_Z);
    end_line = base::add_sat(end_line, 1_Z);

    start_line = std::clamp(start_line, 0_Z, tree.line_count());
    end_line = std::clamp(end_line, 0_Z, tree.line_count());

    auto& text_renderer = Renderer::instance().getTextRenderer();
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();

#ifdef ENABLE_HIGHLIGHTING
    auto& highlighter = highlight::Highlighter::instance();
    auto& language = highlighter.getLanguage("cpp");
    std::vector<highlight::Highlight> highlights;
    {
        PROFILE_BLOCK("SyntaxHighlighter::getHighlights()");
        highlights = language.highlight(parse_tree.getTree(), start_line, end_line);
    }
#endif

    // TODO: Refactor code in draw() to only fetch caret [line, col] once.
    size_t selection_line = tree.line_at(selection.end().index);

    PROFILE_BLOCK("TextViewWidget::renderText()");

    for (size_t line = start_line; line < end_line; ++line) {
        const auto& layout = layoutAt(line);

        app::Point coords = textOffset();
        coords.y += static_cast<int>(line) * main_line_height;
        coords.x += kCaretWidth / 2;  // Match Sublime Text.

        int min_x = scroll_offset.x;
        int max_x = scroll_offset.x + size.width;

#ifdef ENABLE_HIGHLIGHTING
        std::stack<highlight::Highlight> stk;
        auto it = highlights.begin();
        const auto highlight_callback = [&](size_t col) -> Rgb {
            TSPoint p = {
                .row = static_cast<uint32_t>(line),
                .column = static_cast<uint32_t>(col),
            };

            // Use stack to parse highlights.
            while (it != highlights.end() && p >= (*it).end) {
                ++it;
            }
            while (it != highlights.end() && (*it).contains(p)) {
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
            if (!stk.empty() && stk.top().contains(p)) {
                capture_index = stk.top().capture_index;

                // TODO: Use unified Rgb struct.
                const auto& highlight_color = language.getColor(capture_index);
                return {.r = highlight_color.r, .g = highlight_color.g, .b = highlight_color.b};
            } else {
                return kTextColor;
            }
        };
#endif

#ifdef ENABLE_HIGHLIGHTING
        text_renderer.renderLineLayout(layout, coords, TextRenderer::TextLayer::kBackground,
                                       highlight_callback, min_x, max_x);
#else
        text_renderer.renderLineLayout(
            layout, coords, TextRenderer::TextLayer::kBackground,
            [](size_t) { return kTextColor; }, min_x, max_x);
#endif

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
        const auto& line_number_layout = line_layout_cache.get(font_id, line_number_str);
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

void TextViewWidget::renderSelections(int main_line_height, size_t start_line, size_t end_line) {
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
    selection_renderer.renderSelections(selections, textOffset(), main_line_height);
}

void TextViewWidget::renderScrollBars(int main_line_height) {
    auto& rect_renderer = Renderer::instance().getRectRenderer();

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
    auto& rect_renderer = Renderer::instance().getRectRenderer();

    int extra_padding = 8;
    int caret_height = main_line_height + extra_padding * 2;

    auto [line, col] = tree.line_column_at(selection.end().index);
    int end_caret_x = Caret::xAtColumn(layoutAt(line), col);

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
