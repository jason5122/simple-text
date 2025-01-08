#include "text_edit_widget.h"

#include "base/numeric/literals.h"
#include "base/numeric/saturation_arithmetic.h"
#include "gui/renderer/renderer.h"
#include "gui/text_system/movement.h"

#include <cmath>
#include <fmt/base.h>
#include <fmt/format.h>

// TODO: Debug use; remove this.
#include "util/profile_util.h"
#include <cassert>

namespace gui {

TextEditWidget::TextEditWidget(std::string_view str8, size_t font_id)
    : font_id(font_id), tree(str8) {
    updateMaxScroll();

#ifdef ENABLE_HIGHLIGHTING
    auto& highlighter = highlight::Highlighter::instance();
    auto& language = highlighter.getLanguage("cpp");
    PROFILE_BLOCK("TextViewWidget: highlighter.parse()");
    parse_tree.parse(tree, language);
#endif
}

void TextEditWidget::selectAll() {
    selection.setRange(0, tree.length());
}

void TextEditWidget::move(MoveBy by, bool forward, bool extend) {
    PROFILE_BLOCK("TextViewWidget::move()");

    auto [line, col] = tree.line_column_at(selection.end());
    const auto& layout = layoutAt(line);

    if (by == MoveBy::kCharacters && !forward) {
        if (!extend && !selection.empty()) {
            selection.collapse(Selection::Direction::kLeft);
            return;
        }

        size_t delta = Movement::moveToPrevGlyph(layout, col);
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

        size_t delta = Movement::moveToNextGlyph(layout, col);
        selection.increment(delta, extend);
    }
    if (by == MoveBy::kLines) {
        size_t new_line = forward ? line + 1 : line - 1;
        if (0 <= new_line && new_line < tree.line_count()) {
            int x = Movement::xAtColumn(layout, col);
            size_t new_col = Movement::columnAtX(layoutAt(new_line), x);
            size_t index = tree.offset_at(new_line, new_col);
            selection.setIndex(index, extend);
        }
    }
    if (by == MoveBy::kWords) {
        if (forward) {
            selection.end() = Movement::nextWordEnd(tree, selection.end());
        } else {
            selection.end() = Movement::prevWordStart(tree, selection.end());
        }
        if (!extend) {
            selection.start() = selection.end();
        }
    }

    if (by == MoveBy::kCharacters) {}
}

void TextEditWidget::moveTo(MoveTo to, bool extend) {
    PROFILE_BLOCK("TextViewWidget::moveTo()");

    if (to == MoveTo::kBOL || to == MoveTo::kHardBOL) {
        size_t line = tree.line_at(selection.end());

        const auto& layout = layoutAt(line);
        size_t new_col = Movement::columnAtX(layout, 0);
        selection.setIndex(tree.offset_at(line, new_col), extend);
    }
    if (to == MoveTo::kEOL || to == MoveTo::kHardEOL) {
        size_t line = tree.line_at(selection.end());

        const auto& layout = layoutAt(line);
        size_t new_col = Movement::columnAtX(layout, layout.width);
        selection.setIndex(tree.offset_at(line, new_col), extend);
    }
    if (to == MoveTo::kBOF) {
        selection.setIndex(0, extend);
    }
    if (to == MoveTo::kEOF) {
        selection.setIndex(tree.length(), extend);
    }
}

void TextEditWidget::insertText(std::string_view str8) {
    if (!selection.empty()) {
        leftDelete();
    }

    size_t i = selection.end();
    tree.insert(i, str8);
    selection.increment(str8.length(), false);

#ifdef ENABLE_HIGHLIGHTING
    PROFILE_BLOCK("TextViewWidget::insertText() edit + parse");
    auto& highlighter = highlight::Highlighter::instance();
    auto& language = highlighter.getLanguage("cpp");
    parse_tree.edit(i, i, i + str8.length());
    parse_tree.parse(tree, language);
#endif

    // TODO: Do we update caret `max_x` too?

    updateMaxScroll();
}

void TextEditWidget::leftDelete() {
    PROFILE_BLOCK("TextViewWidget::leftDelete()");

    if (selection.empty()) {
        auto [line, col] = tree.line_column_at(selection.end());
        const auto& layout = layoutAt(line);

        size_t delta = Movement::moveToPrevGlyph(layout, col);
        selection.decrement(delta, false);

        // Delete newline if at beginning of line.
        if (delta == 0 && line > 0) {
            selection.decrement(1_Z, false);
            delta = 1;
        }

        size_t i = selection.end();
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

void TextEditWidget::rightDelete() {
    PROFILE_BLOCK("TextViewWidget::rightDelete()");

    if (selection.empty()) {
        auto [line, col] = tree.line_column_at(selection.end());
        const auto& layout = layoutAt(line);

        size_t delta = Movement::moveToNextGlyph(layout, col);
        size_t i = selection.end();
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
void TextEditWidget::deleteWord(bool forward) {
    PROFILE_BLOCK("TextViewWidget::deleteWord()");

    if (selection.empty()) {
        size_t prev_offset = selection.end();
        size_t offset, delta;
        if (forward) {
            offset = Movement::nextWordEnd(tree, prev_offset);
            delta = offset - prev_offset;
            tree.erase(prev_offset, delta);

            // TODO: Clean up selection/caret code.
            // TODO: After clean up, move this out of TextViewWidget.
            selection.end() = prev_offset;
            selection.start() = selection.end();
        } else {
            offset = Movement::prevWordStart(tree, prev_offset);
            delta = prev_offset - offset;
            tree.erase(offset, delta);

            // TODO: Clean up selection/caret code.
            // TODO: After clean up, move this out of TextViewWidget.
            selection.end() = offset;
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

std::string TextEditWidget::getSelectionText() {
    auto [start, end] = selection.range();
    return tree.substr(start, end - start);
}

void TextEditWidget::undo() {
    tree.undo();
}

void TextEditWidget::redo() {
    tree.redo();
}

void TextEditWidget::find(std::string_view str8) {
    std::optional<size_t> result = tree.find(str8);
    if (result) {
        size_t offset = *result;
        selection.setRange(offset, offset + str8.length());
    }
}

// TODO: Use a struct type for clarity.
std::pair<size_t, size_t> TextEditWidget::getLineColumn() {
    size_t offset = selection.end();
    auto cursor = tree.line_column_at(offset);
    return {cursor.line, cursor.column};
}

size_t TextEditWidget::getSelectionLength() {
    return selection.length();
}

void TextEditWidget::updateFontId(size_t font_id) {
    this->font_id = font_id;
    updateMaxScroll();
}

void TextEditWidget::draw() {
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);

    // Calculate start and end lines.
    int main_line_height = metrics.line_height;
    size_t visible_lines = std::ceil(static_cast<double>(size.height) / main_line_height);

    size_t start_line = scroll_offset.y / main_line_height;
    size_t end_line = start_line + visible_lines;

    // Render two lines before start and after end. This ensures no sudden cutoff.
    start_line = base::sub_sat(start_line, 2_Z);
    end_line = base::add_sat(end_line, 2_Z);
    start_line = std::clamp(start_line, 0_Z, tree.line_count());
    end_line = std::clamp(end_line, 0_Z, tree.line_count());

    renderText(main_line_height, start_line, end_line);
    renderSelections(main_line_height, start_line, end_line);
    // Render caret first so scroll bar draws over it.
    renderCaret(main_line_height);
    renderScrollBars(main_line_height);
}

void TextEditWidget::leftMouseDown(const Point& mouse_pos,
                                   ModifierKey modifiers,
                                   ClickType click_type) {
    Point coords = mouse_pos - textOffset();
    size_t line = lineAtY(coords.y);
    size_t col = Movement::columnAtX(layoutAt(line), coords.x);
    size_t offset = tree.offset_at(line, col);

    if (click_type == ClickType::kSingleClick) {
        bool extend = modifiers == ModifierKey::kShift;
        selection.setIndex(offset, extend);
    } else if (click_type == ClickType::kDoubleClick) {
        // TODO: Refine double click implementation.
        size_t left = Movement::prevWordStart(tree, offset);
        size_t right = Movement::nextWordEnd(tree, offset);
        selection.setRange(left, right);
    } else if (click_type == ClickType::kTripleClick) {
        auto [left, right] = tree.get_line_range_with_newline(line);
        selection.setRange(left, right);
    }
}

void TextEditWidget::leftMouseDrag(const Point& mouse_pos,
                                   ModifierKey modifiers,
                                   ClickType click_type) {
    Point coords = mouse_pos - textOffset();
    size_t line = lineAtY(coords.y);
    size_t col = Movement::columnAtX(layoutAt(line), coords.x);
    size_t offset = tree.offset_at(line, col);

    if (click_type == ClickType::kSingleClick) {
        selection.setIndex(offset, true);
    } else if (click_type == ClickType::kDoubleClick) {
        // TODO: Implement.
    } else if (click_type == ClickType::kTripleClick) {
        auto [left, right] = tree.get_line_range_with_newline(line);
        selection.setIndex(right, true);
    }
}

void TextEditWidget::updateMaxScroll() {
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);

    // TODO: Figure out how to update max width.
    max_scroll_offset.x = 2000;
    // max_scroll_offset.x = 0;
    max_scroll_offset.y = tree.line_count() * metrics.line_height;
}

size_t TextEditWidget::lineAtY(int y) const {
    if (y < 0) {
        y = 0;
    }

    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);

    size_t line = y / metrics.line_height;
    return std::clamp(line, 0_Z, tree.line_count() - 1);
}

inline const font::LineLayout& TextEditWidget::layoutAt(size_t line) {
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    std::string line_str = tree.get_line_content_for_layout_use(line);
    return line_layout_cache.get(font_id, line_str);
}

inline constexpr Point TextEditWidget::textOffset() {
    Point text_offset = position - scroll_offset;
    text_offset.x += gutterWidth();
    return text_offset;
}

inline constexpr int TextEditWidget::gutterWidth() {
    return kGutterLeftPadding + lineNumberWidth() + kGutterRightPadding;
}

inline int TextEditWidget::lineNumberWidth() {
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    int digit_width = line_layout_cache.get(font_id, "0").width;
    int log = std::log10(tree.line_count());
    return digit_width * std::max(log + 1, 2);
}

void TextEditWidget::renderText(int main_line_height, size_t start_line, size_t end_line) {
    auto& texture_renderer = Renderer::instance().getTextureRenderer();
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();

    // TODO: Refactor code in draw() to only fetch caret [line, col] once.
    size_t selection_line = tree.line_at(selection.end());

    PROFILE_BLOCK("TextViewWidget::renderText()");

    Point min_text_coords = {
        .x = scroll_offset.x,
        .y = position.y,
    };
    Point max_text_coords = {
        .x = scroll_offset.x + (size.width - (gutterWidth() + kBorderThickness)),
        .y = position.y + size.height,
    };

    {
        // TODO: Implement shadows.
        Point coords = textOffset() + scroll_offset;
        coords.x += kBorderThickness;
        rect_renderer.addRect(coords, {2, size.height}, position, position + size, {255},
                              Layer::kBackground);
        rect_renderer.addRect(coords, {size.width, 2}, position, position + size, {255},
                              Layer::kBackground);
        // coords.x += size.width - (gutterWidth() + kBorderThickness);
        // rect_renderer.addRect(coords, {2, size.height}, position, position + size, {255},
        //                       Layer::kBackground);
    }

    for (size_t line = start_line; line < end_line; ++line) {
        const auto& layout = layoutAt(line);

        Point coords = textOffset();
        coords.y += static_cast<int>(line) * main_line_height;
        coords.x += kBorderThickness;  // Match Sublime Text.

        texture_renderer.addLineLayout(layout, coords, min_text_coords, max_text_coords,
                                       [](size_t) { return kTextColor; });

        // Draw gutter.
        if (line == selection_line) {
            Point gutter_coords = position;
            gutter_coords.y -= scroll_offset.y;
            gutter_coords.y += static_cast<int>(line) * main_line_height;
            Size gutter_size = {gutterWidth(), main_line_height};
            rect_renderer.addRect(gutter_coords, gutter_size, position, position + size,
                                  kGutterColor, Layer::kBackground, 0, 0);
        }

        // Draw line numbers.
        Point line_number_coords = position;
        line_number_coords.y -= scroll_offset.y;
        line_number_coords.x += kGutterLeftPadding;
        line_number_coords.y += static_cast<int>(line) * main_line_height;

        std::string line_number_str = fmt::format("{}", line + 1);
        const auto& line_number_layout = line_layout_cache.get(font_id, line_number_str);
        line_number_coords.x += lineNumberWidth() - line_number_layout.width;

        const auto line_number_highlight_callback = [&line, &selection_line](size_t) {
            return line == selection_line ? kSelectedLineNumberColor : kLineNumberColor;
        };
        Point min_gutter_coords = {
            .x = 0,
            .y = position.y,
        };
        Point max_gutter_coords = {
            .x = gutterWidth(),
            .y = position.y + size.height,
        };
        texture_renderer.addLineLayout(line_number_layout, line_number_coords, min_gutter_coords,
                                       max_gutter_coords, line_number_highlight_callback);
    }
}

void TextEditWidget::renderSelections(int main_line_height, size_t start_line, size_t end_line) {
    auto& selection_renderer = Renderer::instance().getSelectionRenderer();
    auto [start, end] = selection.range();
    auto [c1_line, c1_col] = tree.line_column_at(start);
    auto [c2_line, c2_col] = tree.line_column_at(end);

    const auto& c1_layout = layoutAt(c1_line);
    const auto& c2_layout = layoutAt(c2_line);
    int c1_x = Movement::xAtColumn(c1_layout, c1_col);
    int c2_x = Movement::xAtColumn(c2_layout, c2_col);

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
            if (start > 0) start += kBorderThickness;
            end += kBorderThickness;

            selections.emplace_back(SelectionRenderer::Selection{
                .line = static_cast<int>(line),
                .start = start,
                .end = end,
            });
        }
    }

    Point min_coords = {
        .x = gutterWidth() + position.x,
        .y = position.y,
    };
    Point max_coords = {
        .x = position.x + size.width,
        .y = position.y + size.height,
    };
    selection_renderer.addSelections(selections, textOffset(), main_line_height, min_coords,
                                     max_coords);
}

void TextEditWidget::renderScrollBars(int main_line_height) {
    auto& rect_renderer = Renderer::instance().getRectRenderer();

    // Add vertical scroll bar.
    int vbar_width = 15;
    double max_scrollbar_y = size.height + tree.line_count() * main_line_height;
    double vbar_height_percent = static_cast<double>(size.height) / max_scrollbar_y;
    int vbar_height = static_cast<int>(size.height * vbar_height_percent);
    vbar_height = std::max(30, vbar_height);
    double vbar_percent = static_cast<double>(scroll_offset.y) / max_scroll_offset.y;

    Point vbar_coords = {
        .x = size.width - vbar_width,
        .y = static_cast<int>(std::round((size.height - vbar_height) * vbar_percent)),
    };
    vbar_coords += position;
    Size vbar_size = {vbar_width, vbar_height};
    rect_renderer.addRect(vbar_coords, vbar_size, position, position + size, kScrollBarColor,
                          Layer::kForeground, 5);

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

void TextEditWidget::renderCaret(int main_line_height) {
    auto& rect_renderer = Renderer::instance().getRectRenderer();

    int caret_height = main_line_height + kExtraPadding * 2;

    auto [line, col] = tree.line_column_at(selection.end());
    int end_caret_x = Movement::xAtColumn(layoutAt(line), col);

    Point caret_pos = {
        .x = end_caret_x,
        .y = static_cast<int>(line) * main_line_height,
    };
    caret_pos.y -= kExtraPadding;
    caret_pos += textOffset();
    Size caret_size = {kCaretWidth, caret_height};

    Point min_coords = {
        .x = gutterWidth() + position.x,
        .y = position.y,
    };
    Point max_coords = {
        .x = position.x + size.width,
        .y = position.y + size.height,
    };
    rect_renderer.addRect(caret_pos, caret_size, min_coords, max_coords, kCaretColor,
                          Layer::kForeground, 0, 0);
}

}  // namespace gui
