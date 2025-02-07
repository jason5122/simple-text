#include "text_button_widget.h"

#include "gui/renderer/renderer.h"

#include <fmt/base.h>

namespace gui {

TextButtonWidget::TextButtonWidget(
    size_t font_id, std::string_view str8, Rgb bg_color, const Size& padding, const Size& min_size)
    : bg_color(bg_color) {
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);
    line_height = metrics.line_height;

    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    line_layout = line_layout_cache.get(font_id, str8);

    set_width(std::max(line_layout.width + padding.width * 2, min_size.width));
    set_height(std::max(line_height + padding.height * 2, min_size.height));
}

void TextButtonWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    auto& texture_renderer = Renderer::instance().getTextureRenderer();

    rect_renderer.addRect(position(), size(), position(), position() + size(), bg_color,
                          Layer::kBackground, 4);

    Point min_coords = {
        .x = 0,
        .y = position().y,
    };
    Point max_coords = {
        .x = size().width,
        .y = position().y + size().height,
    };
    texture_renderer.addLineLayout(line_layout, text_center(), min_coords, max_coords,
                                   [](size_t) { return kTextColor; });
}

constexpr Point TextButtonWidget::text_center() {
    auto pos = position();
    pos.x += size().width / 2;
    pos.x -= line_layout.width / 2;
    pos.y += size().height / 2;
    pos.y -= line_height / 2;
    return pos;
}

}  // namespace gui
