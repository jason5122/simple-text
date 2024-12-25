#include "text_button_widget.h"

#include "gui/renderer/renderer.h"

#include <fmt/base.h>

namespace gui {

// TODO: Accept a "minimum size" argument.
TextButtonWidget::TextButtonWidget(size_t font_id, std::string_view str8, Rgba bg_color)
    : bg_color(bg_color) {
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);
    line_height = metrics.line_height;

    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    line_layout = line_layout_cache.get(font_id, str8);

    setWidth(line_layout.width);
    // setHeight(line_height);
    setHeight(std::max(line_height + 8 * 2, 52));

    fmt::println("regular UI line height = {}", line_height);
    fmt::println("line_layout.width = {}", line_layout.width);
}

void TextButtonWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    auto& text_renderer = Renderer::instance().getTextRenderer();

    rect_renderer.addRect(position, size, bg_color, Layer::kTwo, 4);
    app::Point pos = {
        .x = centerHorizontally(line_layout.width),
        .y = centerVertically(line_height),
    };
    text_renderer.renderLineLayout(
        line_layout, pos, Layer::kTwo, [](size_t) { return kTextColor; }, 0, size.width);
}

int TextButtonWidget::centerHorizontally(int widget_width) {
    int centered_x = position.x;
    centered_x += size.width / 2;
    centered_x -= widget_width / 2;
    return centered_x;
}

int TextButtonWidget::centerVertically(int widget_height) {
    int centered_y = position.y;
    centered_y += size.height / 2;
    centered_y -= widget_height / 2;
    return centered_y;
}

}  // namespace gui
