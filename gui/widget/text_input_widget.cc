#include "text_input_widget.h"

#include "gui/renderer/renderer.h"
#include "gui/text_system/movement.h"

#include <fmt/base.h>

namespace gui {

TextInputWidget::TextInputWidget(size_t font_id, int top_padding, int left_padding)
    : font_id(font_id), top_padding(top_padding), left_padding(left_padding) {
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

    auto pos = position;
    pos.y += top_padding;
    pos.x += left_padding;
    pos.x += kCaretWidth / 2;  // Match Sublime Text.

    Point min_text_coords = {
        .x = 0,
        .y = position.y,
    };
    Point max_text_coords = {
        .x = size.width,
        .y = position.y + size.height,
    };

    auto& texture_renderer = Renderer::instance().getTextureRenderer();
    texture_renderer.addLineLayout(getLayout(), pos, min_text_coords, max_text_coords,
                                   [](size_t) { return kTextColor; });

    int caret_x = Movement::xAtColumn(getLayout(), caret);
    Point caret_pos = pos;
    caret_pos.x += caret_x;
    Size caret_size = {kCaretWidth, line_height};
    rect_renderer.addRect(caret_pos, caret_size, position, position + size, kCaretColor,
                          Layer::kBackground);
    // fmt::println("caret_x = {}", caret_x);
}

void TextInputWidget::updateMaxScroll() {
    ;
}

void TextInputWidget::leftMouseDown(const Point& mouse_pos,
                                    ModifierKey modifiers,
                                    ClickType click_type) {
    // TODO: Refactor this.
    auto pos = position;
    pos.x += left_padding;
    // pos.x += 2;  // Caret width.

    auto coords = mouse_pos - pos;
    caret = Movement::columnAtX(getLayout(), coords.x);
    // fmt::println("caret = {}", caret);
}

inline const font::LineLayout& TextInputWidget::getLayout() const {
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    return line_layout_cache.get(font_id, find_str);
}

}  // namespace gui
