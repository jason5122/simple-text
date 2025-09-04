#include "base/numeric/safe_conversions.h"
#include "editor/movement.h"
#include "gui/renderer/renderer.h"
#include "gui/widget/text_input_widget.h"

namespace gui {

TextInputWidget::TextInputWidget(size_t font_id, int top_padding, int left_padding)
    : font_id(font_id), top_padding(top_padding), left_padding(left_padding) {

    update_max_scroll();

    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);
    line_height = metrics.line_height;

    int new_height = line_height;
    new_height = line_height;
    new_height += top_padding * 2;
    new_height += 2;  // Selection width.
    set_height(new_height);
}

void TextInputWidget::draw() {
    auto& rect_renderer = Renderer::instance().rect_renderer();
    auto& texture_renderer = Renderer::instance().texture_renderer();

    rect_renderer.add_rect(position(), size(), position(), position() + size(), kBackgroundColor,
                           Layer::kBackground, 4);

    // auto pos = position;
    // pos.y += top_padding;
    // pos.x += left_padding;
    // pos.x += kCaretWidth / 2;  // Match Sublime Text.

    Point min_text_coords = {
        .x = scroll_offset.x - kBorderThickness,
        .y = position().y,
    };
    Point max_text_coords = {
        .x = scroll_offset.x + (size().width - (left_padding + kBorderThickness)),
        .y = position().y + size().height,
    };

    // We set the max horizontal scroll to the max width out of each *visible* line. This means the
    // max horizontal scroll changes dynamically as the user scrolls through the buffer, but this
    // is better than computing it up front and having to update it.
    //
    // As long as the user can scroll to the section vertically and then scroll horizontally once
    // there, it shouldn't be confusing or limiting at all in my opinion.
    int max_layout_width = 0;

    for (size_t line = 0; line < tree.line_count(); ++line) {
        const auto& layout = layout_at(line);

        max_layout_width = std::max(layout.width, max_layout_width);

        Point coords = text_offset();
        coords.y += static_cast<int>(line) * line_height;
        coords.x += kBorderThickness;  // Match Sublime Text.

        texture_renderer.add_line_layout(layout, coords, min_text_coords, max_text_coords,
                                         [](size_t) { return kTextColor; });
    }

    max_scroll_offset.x = max_layout_width;

    auto [line, col] = tree.line_column_at(selection.end);
    int end_caret_x = editor::x_at_column(layout_at(line), col);

    {
        Point caret_pos = {
            .x = end_caret_x,
            .y = static_cast<int>(line) * line_height,
        };
        caret_pos += text_offset();
        Size caret_size = {kCaretWidth, line_height};

        Point min_coords = {
            .x = left_padding + position().x,
            .y = position().y,
        };
        Point max_coords = {
            .x = position().x + width(),
            .y = position().y + height(),
        };

        rect_renderer.add_rect(caret_pos, caret_size, min_coords, max_coords, kCaretColor,
                               Layer::kForeground);
    }
}

void TextInputWidget::update_max_scroll() {
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);

    // NOTE: We update the max width when iterating over visible lines, not here.

    max_scroll_offset.y = base::checked_cast<int>(tree.line_count()) * metrics.line_height;
}

void TextInputWidget::left_mouse_down(const Point& mouse_pos,
                                      ModifierKey modifiers,
                                      ClickType click_type) {
    auto coords = mouse_pos - text_offset();
    size_t line = line_at_y(coords.y);
    size_t col = editor::column_at_x(layout_at(line), coords.x);
    size_t offset = tree.offset_at(line, col);
    selection.set_index(offset, true);
}

void TextInputWidget::insert_text(std::string_view str8) {
    size_t i = selection.end;
    tree.insert(i, str8);
    selection.increment(str8.length(), false);

    update_max_scroll();
}

size_t TextInputWidget::line_at_y(int y) const {
    if (y < 0) y = 0;

    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);

    size_t line = y / metrics.line_height;
    return std::clamp(line, size_t{0}, tree.line_count() - 1);
}

inline const font::LineLayout& TextInputWidget::layout_at(size_t line) {
    auto& line_layout_cache = Renderer::instance().line_layout_cache();
    std::string line_str = tree.get_line_content_for_layout_use(line);
    return line_layout_cache.get(font_id, line_str);
}

inline constexpr Point TextInputWidget::text_offset() {
    Point text_offset = position() - scroll_offset;
    text_offset.x += left_padding;
    text_offset.y += top_padding;
    return text_offset;
}

}  // namespace gui
