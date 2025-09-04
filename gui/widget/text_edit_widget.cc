#include "base/debug/profiler.h"
#include "base/numeric/safe_conversions.h"
#include "base/numeric/saturation_arithmetic.h"
#include "editor/movement.h"
#include "gui/renderer/renderer.h"
#include "gui/widget/text_edit_widget.h"
#include <cassert>
#include <cmath>
#include <fmt/format.h>

namespace gui {

TextEditWidget::TextEditWidget(std::string_view str8, size_t font_id)
    : font_id(font_id), tree(str8) {
    update_max_scroll();
}

void TextEditWidget::select_all() { selection.set_range(0, tree.length()); }

void TextEditWidget::move(MoveBy by, bool forward, bool extend) {
    auto p = base::Profiler{"TextViewWidget::move()"};

    auto [line, col] = tree.line_column_at(selection.end);
    const auto& layout = layout_at(line);

    switch (by) {
    case MoveBy::kCharacters: {
        if (forward) {
            if (!extend && !selection.empty()) {
                selection.collapse_right();
                return;
            }

            size_t delta = editor::move_to_next_glyph(layout, col);
            selection.increment(delta, extend);
        } else {
            if (!extend && !selection.empty()) {
                selection.collapse_left();
                return;
            }

            size_t delta = editor::move_to_prev_glyph(layout, col);
            selection.decrement(delta, extend);

            // Move to previous line if at beginning of line.
            if (delta == 0 && line > 0) {
                const auto& prev_layout = layout_at(line - 1);
                size_t index =
                    tree.offset_at(line - 1, base::sub_sat(prev_layout.length, size_t{1}));
                selection.set_index(index, extend);
            }
        }
        break;
    }
    case MoveBy::kLines: {
        size_t new_line = forward ? line + 1 : line - 1;
        if (0 <= new_line && new_line < tree.line_count()) {
            int x = editor::x_at_column(layout, col);
            size_t new_col = editor::column_at_x(layout_at(new_line), x);
            size_t index = tree.offset_at(new_line, new_col);
            selection.set_index(index, extend);
        }
        break;
    }
    case MoveBy::kWords: {
        if (forward) {
            selection.end = editor::next_word_end(tree, selection.end);
        } else {
            selection.end = editor::prev_word_start(tree, selection.end);
        }
        if (!extend) {
            selection.start = selection.end;
        }
        break;
    }
    }
}

void TextEditWidget::move_to(MoveTo to, bool extend) {
    auto p = base::Profiler{"TextViewWidget::moveTo()"};

    switch (to) {
    case MoveTo::kBOL:
    case MoveTo::kHardBOL: {
        size_t line = tree.line_at(selection.end);
        const auto& layout = layout_at(line);
        size_t new_col = editor::column_at_x(layout, 0);
        selection.set_index(tree.offset_at(line, new_col), extend);
        break;
    }
    case MoveTo::kEOL:
    case MoveTo::kHardEOL: {
        size_t line = tree.line_at(selection.end);
        const auto& layout = layout_at(line);
        size_t new_col = editor::column_at_x(layout, layout.width);
        selection.set_index(tree.offset_at(line, new_col), extend);
        break;
    }
    case MoveTo::kBOF: {
        selection.set_index(0, extend);
        break;
    }
    case MoveTo::kEOF: {
        selection.set_index(tree.length(), extend);
        break;
    }
    }
}

void TextEditWidget::insert_text(std::string_view str8) {
    if (!selection.empty()) {
        left_delete();
    }

    size_t i = selection.end;
    tree.insert(i, str8);
    selection.increment(str8.length(), false);

    // TODO: Do we update caret `max_x` too?

    update_max_scroll();
}

void TextEditWidget::left_delete() {
    auto p = base::Profiler{"TextViewWidget::leftDelete()"};

    if (selection.empty()) {
        auto [line, col] = tree.line_column_at(selection.end);
        const auto& layout = layout_at(line);

        size_t delta = editor::move_to_prev_glyph(layout, col);
        selection.decrement(delta, false);

        // Delete newline if at beginning of line.
        if (delta == 0 && line > 0) {
            selection.decrement(size_t{1}, false);
            delta = 1;
        }

        size_t i = selection.end;
        tree.erase(i, delta);
    } else {
        auto [start, end] = selection.range();
        tree.erase(start, end - start);
        selection.collapse_left();
    }

    update_max_scroll();
}

void TextEditWidget::right_delete() {
    auto p = base::Profiler{"TextViewWidget::rightDelete()"};

    if (selection.empty()) {
        auto [line, col] = tree.line_column_at(selection.end);
        const auto& layout = layout_at(line);

        size_t delta = editor::move_to_next_glyph(layout, col);
        size_t i = selection.end;
        tree.erase(i, delta);
    } else {
        auto [start, end] = selection.range();
        tree.erase(start, end - start);
        selection.collapse_left();
    }

    update_max_scroll();
}

// TODO: Make this delete newlines without going past them into the previous line.
void TextEditWidget::delete_word(bool forward) {
    auto p = base::Profiler{"TextViewWidget::deleteWord()"};

    if (selection.empty()) {
        size_t prev_offset = selection.end;
        size_t offset, delta;
        if (forward) {
            offset = editor::next_word_end(tree, prev_offset);
            delta = offset - prev_offset;
            tree.erase(prev_offset, delta);

            // TODO: Clean up selection/caret code.
            // TODO: After clean up, move this out of TextViewWidget.
            selection.end = prev_offset;
            selection.start = selection.end;
        } else {
            offset = editor::prev_word_start(tree, prev_offset);
            delta = prev_offset - offset;
            tree.erase(offset, delta);

            // TODO: Clean up selection/caret code.
            // TODO: After clean up, move this out of TextViewWidget.
            selection.end = offset;
            selection.start = selection.end;
        }
    } else {
        auto [start, end] = selection.range();
        tree.erase(start, end - start);
        selection.collapse_left();
    }
}

std::string TextEditWidget::get_selection_text() {
    auto [start, end] = selection.range();
    return tree.substr(start, end - start);
}

void TextEditWidget::undo() { tree.undo(); }

void TextEditWidget::redo() { tree.redo(); }

void TextEditWidget::find(std::string_view str8) {
    std::optional<size_t> result = tree.find(str8);
    if (result) {
        size_t offset = *result;
        selection.set_range(offset, offset + str8.length());
    }
}

// TODO: Use a struct type for clarity.
std::pair<size_t, size_t> TextEditWidget::get_line_column() {
    size_t offset = selection.end;
    auto cursor = tree.line_column_at(offset);
    return {cursor.line, cursor.column};
}

size_t TextEditWidget::get_selection_length() { return selection.length(); }

void TextEditWidget::update_font_id(size_t font_id) {
    this->font_id = font_id;
    update_max_scroll();
}

void TextEditWidget::draw() {
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);

    // Calculate start and end lines.
    int main_line_height = metrics.line_height;
    size_t visible_lines = std::ceil(static_cast<double>(size().height) / main_line_height);

    size_t start_line = scroll_offset.y / main_line_height;
    size_t end_line = start_line + visible_lines;

    // Render two lines before start and after end. This ensures no sudden cutoff.
    start_line = base::sub_sat(start_line, size_t{2});
    end_line = base::add_sat(end_line, size_t{2});
    start_line = std::clamp(start_line, size_t{0}, tree.line_count());
    end_line = std::clamp(end_line, size_t{0}, tree.line_count());

    render_text(main_line_height, start_line, end_line);
    render_selections(main_line_height, start_line, end_line);
    // Render caret first so scroll bar draws over it.
    render_caret(main_line_height);
    render_scroll_bars(main_line_height);
}

void TextEditWidget::left_mouse_down(const Point& mouse_pos,
                                     ModifierKey modifiers,
                                     ClickType click_type) {
    Point coords = mouse_pos - text_offset();
    size_t line = line_at_y(coords.y);
    size_t col = editor::column_at_x(layout_at(line), coords.x);
    size_t offset = tree.offset_at(line, col);

    switch (click_type) {
    case ClickType::kSingleClick: {
        bool extend = modifiers == ModifierKey::kShift;
        selection.set_index(offset, extend);
        break;
    }
    case ClickType::kDoubleClick: {
        auto [left, right] = editor::surrounding_word(tree, offset);
        selection.set_range(left, right);
        old_selection = selection;
        break;
    }
    case ClickType::kTripleClick: {
        // TODO: Implement extending.
        auto [left, right] = tree.get_line_range_with_newline(line);
        selection.set_range(left, right);
        break;
    }
    }
}

void TextEditWidget::left_mouse_drag(const Point& mouse_pos,
                                     ModifierKey modifiers,
                                     ClickType click_type) {
    Point coords = mouse_pos - text_offset();
    size_t line = line_at_y(coords.y);
    size_t col = editor::column_at_x(layout_at(line), coords.x);
    size_t offset = tree.offset_at(line, col);

    switch (click_type) {
    case ClickType::kSingleClick: {
        selection.set_index(offset, true);
        break;
    }
    case ClickType::kDoubleClick: {
        if (editor::is_inside_word(tree, offset)) {
            auto [left, right] = editor::surrounding_word(tree, offset);
            left = std::min(left, old_selection.start);
            right = std::max(right, old_selection.end);
            selection.set_range(left, right);
        } else {
            selection.start = std::min(offset, old_selection.start);
            selection.end = std::max(offset, old_selection.end);
        }
        break;
    }
    case ClickType::kTripleClick: {
        auto [left, right] = tree.get_line_range_with_newline(line);
        selection.set_index(right, true);
        break;
    }
    }
}

void TextEditWidget::left_mouse_up(const Point& mouse_pos) { old_selection = selection; }

void TextEditWidget::update_max_scroll() {
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);

    // NOTE: We update the max width when iterating over visible lines, not here.

    max_scroll_offset.y = base::checked_cast<int>(tree.line_count()) * metrics.line_height;
}

size_t TextEditWidget::line_at_y(int y) const {
    if (y < 0) {
        y = 0;
    }

    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);

    size_t line = y / metrics.line_height;
    return std::clamp(line, size_t{0}, tree.line_count() - 1);
}

inline const font::LineLayout& TextEditWidget::layout_at(size_t line) {
    auto& line_layout_cache = Renderer::instance().line_layout_cache();
    std::string line_str = tree.get_line_content_for_layout_use(line);
    return line_layout_cache.get(font_id, line_str);
}

inline constexpr Point TextEditWidget::text_offset() {
    Point text_offset = position() - scroll_offset;
    text_offset.x += gutter_width();
    return text_offset;
}

inline constexpr int TextEditWidget::gutter_width() {
    return kGutterLeftPadding + line_number_width() + kGutterRightPadding;
}

inline int TextEditWidget::line_number_width() {
    auto& line_layout_cache = Renderer::instance().line_layout_cache();
    int digit_width = line_layout_cache.get(font_id, "0").width;
    int log = std::log10(tree.line_count());
    return digit_width * std::max(log + 1, 2);
}

void TextEditWidget::render_text(int main_line_height, size_t start_line, size_t end_line) {
    auto& texture_renderer = Renderer::instance().texture_renderer();
    auto& rect_renderer = Renderer::instance().rect_renderer();
    auto& line_layout_cache = Renderer::instance().line_layout_cache();

    // TODO: Refactor code in draw() to only fetch caret [line, col] once.
    size_t selection_line = tree.line_at(selection.end);

    auto p = base::Profiler{"TextViewWidget::renderText()"};

    // Draw shadow to indicate horizontal scrolling is possible.
    static constexpr int kShadowWidth = 10;
    if (scroll_offset.x > 0) {
        Point shadow_coords = text_offset() + scroll_offset;
        Size shadow_size = {kShadowWidth, size().height};
        rect_renderer.add_rect(shadow_coords, shadow_size, position(), position() + size(),
                               kShadowColor, Layer::kForeground, 0, 0, true);
    }
    if (scroll_offset.x < max_scroll_offset.x) {
        Point shadow_coords = position();
        shadow_coords.x += size().width - kShadowWidth;
        Size shadow_size = {kShadowWidth, size().height};
        rect_renderer.add_rect(shadow_coords, shadow_size, position(), position() + size(),
                               kShadowColor, Layer::kForeground, 0, 0, false, true);
    }

    Point min_text_coords = {
        .x = scroll_offset.x - kBorderThickness,
        .y = position().y,
    };
    Point max_text_coords = {
        .x = scroll_offset.x + (size().width - (gutter_width() + kBorderThickness)),
        .y = position().y + size().height,
    };

    // We set the max horizontal scroll to the max width out of each *visible* line. This means the
    // max horizontal scroll changes dynamically as the user scrolls through the buffer, but this
    // is better than computing it up front and having to update it.
    //
    // As long as the user can scroll to the section vertically and then scroll horizontally once
    // there, it shouldn't be confusing or limiting at all in my opinion.
    int max_layout_width = 0;

    for (size_t line = start_line; line < end_line; ++line) {
        const auto& layout = layout_at(line);

        max_layout_width = std::max(layout.width, max_layout_width);

        Point coords = text_offset();
        coords.y += static_cast<int>(line) * main_line_height;
        coords.x += kBorderThickness;  // Match Sublime Text.

        texture_renderer.add_line_layout(layout, coords, min_text_coords, max_text_coords,
                                         [](size_t) { return kTextColor; });

        // Draw gutter.
        if (line == selection_line) {
            Point gutter_coords = position();
            gutter_coords.y -= scroll_offset.y;
            gutter_coords.y += static_cast<int>(line) * main_line_height;
            Size gutter_size = {gutter_width(), main_line_height};
            rect_renderer.add_rect(gutter_coords, gutter_size, position(), position() + size(),
                                   kGutterColor, Layer::kBackground);
        }

        // Draw line numbers.
        Point line_number_coords = position();
        line_number_coords.y -= scroll_offset.y;
        line_number_coords.x += kGutterLeftPadding;
        line_number_coords.y += static_cast<int>(line) * main_line_height;

        std::string line_number_str = fmt::format("{}", line + 1);
        const auto& line_number_layout = line_layout_cache.get(font_id, line_number_str);
        line_number_coords.x += line_number_width() - line_number_layout.width;

        const auto line_number_highlight_callback = [&line, &selection_line](size_t) {
            return line == selection_line ? kSelectedLineNumberColor : kLineNumberColor;
        };
        Point min_gutter_coords = {
            .x = 0,
            .y = position().y,
        };
        Point max_gutter_coords = {
            .x = gutter_width(),
            .y = position().y + size().height,
        };
        texture_renderer.add_line_layout(line_number_layout, line_number_coords, min_gutter_coords,
                                         max_gutter_coords, line_number_highlight_callback);
    }

    max_scroll_offset.x = max_layout_width;
}

void TextEditWidget::render_selections(int main_line_height, size_t start_line, size_t end_line) {
    auto& selection_renderer = Renderer::instance().selection_renderer();
    auto [start, end] = selection.range();
    auto [c1_line, c1_col] = tree.line_column_at(start);
    auto [c2_line, c2_col] = tree.line_column_at(end);

    const auto& c1_layout = layout_at(c1_line);
    const auto& c2_layout = layout_at(c2_line);
    int c1_x = editor::x_at_column(c1_layout, c1_col);
    int c2_x = editor::x_at_column(c2_layout, c2_col);

    // Don't render off-screen selections.
    if (c1_line < start_line) c1_line = start_line;
    if (c2_line > end_line) c2_line = end_line;

    std::vector<SelectionRenderer::Selection> selections;
    for (size_t line = c1_line; line <= c2_line; ++line) {
        const auto& layout = layout_at(line);
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
        .x = position().x + gutter_width(),
        .y = position().y,
    };
    Point max_coords = {
        .x = position().x + size().width,
        .y = position().y + size().height,
    };
    selection_renderer.add_selections(selections, text_offset(), main_line_height, min_coords,
                                      max_coords);
}

// TODO: Implement a non-"scroll past end" mode.
void TextEditWidget::render_scroll_bars(int main_line_height) {
    auto& rect_renderer = Renderer::instance().rect_renderer();

    // Add vertical scroll bar.
    // TODO: Consider subtracting 1 from the line count.
    if (tree.line_count() > 0) {
        int vbar_width = kScrollBarThickness;
        double max_scrollbar_y = size().height + tree.line_count() * main_line_height;
        double vbar_height_percent = static_cast<double>(size().height) / max_scrollbar_y;
        int vbar_height = size().height * vbar_height_percent;
        vbar_height = std::max(kMinScrollBarHeight, vbar_height);
        double vbar_percent = static_cast<double>(scroll_offset.y) / max_scroll_offset.y;

        Point vbar_coords = {
            .x = size().width - vbar_width - kScrollBarPadding,
            .y = static_cast<int>(std::round((size().height - vbar_height) * vbar_percent)),
        };
        vbar_coords += position();
        Size vbar_size = {vbar_width, vbar_height};
        rect_renderer.add_rect(vbar_coords, vbar_size, position(), position() + size(),
                               kScrollBarColor, Layer::kForeground, 5);
    }

    // Add horizontal scroll bar.
    // TODO: We shouldn't implement "scroll past end" horizontally.
    if (max_scroll_offset.x > 0) {
        int hbar_height = kScrollBarThickness;
        double max_scrollbar_x = size().width + max_scroll_offset.x;
        double hbar_width_percent = static_cast<double>(size().width) / max_scrollbar_x;
        int hbar_width = size().width * hbar_width_percent;
        hbar_width = std::max(kMinScrollBarWidth, hbar_width);
        double hbar_percent = static_cast<double>(scroll_offset.x) / max_scroll_offset.x;
        Point hbar_coords = {
            .x = static_cast<int>(std::round((size().width - hbar_width) * hbar_percent)),
            .y = size().height - hbar_height - kScrollBarPadding,
        };
        hbar_coords += position();
        Size hbar_size = {hbar_width, hbar_height};
        rect_renderer.add_rect(hbar_coords, hbar_size, position(), position() + size(),
                               kScrollBarColor, Layer::kForeground, 5);
    }
}

void TextEditWidget::render_caret(int main_line_height) {
    auto& rect_renderer = Renderer::instance().rect_renderer();

    int caret_height = main_line_height + kExtraPadding * 2;

    auto [line, col] = tree.line_column_at(selection.end);
    int end_caret_x = editor::x_at_column(layout_at(line), col);

    Point caret_pos = {
        .x = end_caret_x,
        .y = static_cast<int>(line) * main_line_height,
    };
    caret_pos.y -= kExtraPadding;
    caret_pos += text_offset();
    Size caret_size = {kCaretWidth, caret_height};

    Point min_coords = {
        .x = position().x + gutter_width(),
        .y = position().y,
    };
    Point max_coords = {
        .x = position().x + size().width,
        .y = position().y + size().height,
    };
    rect_renderer.add_rect(caret_pos, caret_size, min_coords, max_coords, kCaretColor,
                           Layer::kForeground, 0, 0);
}

}  // namespace gui
