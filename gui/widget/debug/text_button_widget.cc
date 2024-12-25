#include "text_button_widget.h"

#include "gui/renderer/renderer.h"

namespace gui {

// TODO: Accept a "minimum size" argument.
TextButtonWidget::TextButtonWidget(size_t font_id, std::string_view str8, Rgba bg_color)
    : bg_color(bg_color) {
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);
    int line_height = metrics.line_height;

    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    line_layout = line_layout_cache.get(font_id, str8);

    setWidth(line_layout.width);
    setHeight(line_height);
}

void TextButtonWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    auto& text_renderer = Renderer::instance().getTextRenderer();

    rect_renderer.addRect(position, size, bg_color, Layer::kTwo, 4);
    text_renderer.renderLineLayout(
        line_layout, position, Layer::kTwo, [](size_t) { return kTextColor; }, 0, size.width);
}

}  // namespace gui
