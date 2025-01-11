#include "text_input_widget.h"

#include "base/numeric/literals.h"
#include "gui/renderer/renderer.h"
#include "gui/text_system/movement.h"

#include <fmt/base.h>

namespace gui {

TextInputWidget::TextInputWidget(size_t font_id, int top_padding, int left_padding)
    : font_id(font_id), top_padding(top_padding), left_padding(left_padding) {

    updateMaxScroll();

    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);
    line_height = metrics.line_height;

    size.height = line_height;
    size.height += top_padding * 2;
    size.height += 2;  // Selection width.
}

void TextInputWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position, size, position, position + size, kBackgroundColor,
                          Layer::kBackground, 4);

    // auto pos = position;
    // pos.y += top_padding;
    // pos.x += left_padding;
    // pos.x += kCaretWidth / 2;  // Match Sublime Text.

    Point min_text_coords = {
        .x = scroll_offset.x - kBorderThickness,
        .y = position.y,
    };
    Point max_text_coords = {
        .x = scroll_offset.x + (size.width - (left_padding + kBorderThickness)),
        .y = position.y + size.height,
    };

    auto& texture_renderer = Renderer::instance().getTextureRenderer();

    for (size_t line = 0; line < tree.line_count(); ++line) {
        const auto& layout = layoutAt(line);

        Point coords = textOffset();
        coords.y += static_cast<int>(line) * line_height;
        coords.x += kBorderThickness;  // Match Sublime Text.

        texture_renderer.addLineLayout(layout, coords, min_text_coords, max_text_coords,
                                       [](size_t) { return kTextColor; });
    }

    auto [line, col] = tree.line_column_at(selection.end());
    int end_caret_x = movement::xAtColumn(layoutAt(line), col);

    {
        Point caret_pos = {
            .x = end_caret_x,
            .y = static_cast<int>(line) * line_height,
        };
        caret_pos += textOffset();
        Size caret_size = {kCaretWidth, line_height};

        Point min_coords = {
            .x = left_padding + position.x,
            .y = position.y,
        };
        Point max_coords = {
            .x = position.x + size.width,
            .y = position.y + size.height,
        };

        rect_renderer.addRect(caret_pos, caret_size, min_coords, max_coords, kCaretColor,
                              Layer::kForeground);
    }
}

void TextInputWidget::updateMaxScroll() {
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);

    // TODO: Figure out how to update max width.
    max_scroll_offset.x = 2000;
    // max_scroll_offset.x = 0;
    max_scroll_offset.y = tree.line_count() * metrics.line_height;
}

void TextInputWidget::leftMouseDown(const Point& mouse_pos,
                                    ModifierKey modifiers,
                                    ClickType click_type) {
    auto coords = mouse_pos - textOffset();
    size_t line = lineAtY(coords.y);
    size_t col = movement::columnAtX(layoutAt(line), coords.x);
    size_t offset = tree.offset_at(line, col);
    selection.setIndex(offset, true);
}

void TextInputWidget::insertText(std::string_view str8) {
    size_t i = selection.end();
    tree.insert(i, str8);
    selection.increment(str8.length(), false);

    updateMaxScroll();
}

size_t TextInputWidget::lineAtY(int y) const {
    if (y < 0) {
        y = 0;
    }

    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);

    size_t line = y / metrics.line_height;
    return std::clamp(line, 0_Z, tree.line_count() - 1);
}

inline const font::LineLayout& TextInputWidget::layoutAt(size_t line) {
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    std::string line_str = tree.get_line_content_for_layout_use(line);
    return line_layout_cache.get(font_id, line_str);
}

inline constexpr Point TextInputWidget::textOffset() {
    Point text_offset = position - scroll_offset;
    text_offset.x += left_padding;
    text_offset.y += top_padding;
    return text_offset;
}

}  // namespace gui
