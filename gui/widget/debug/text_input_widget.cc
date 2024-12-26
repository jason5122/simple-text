#include "text_input_widget.h"

#include "gui/renderer/renderer.h"

#include <fmt/base.h>

namespace gui {

TextInputWidget::TextInputWidget(size_t font_id) : font_id(font_id) {
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);
    line_height = metrics.line_height;

    size.height = line_height;
    size.height += 8 * 2;
    size.height += 2;
}

void TextInputWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    auto& text_renderer = Renderer::instance().getTextRenderer();

    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    const auto& layout = line_layout_cache.get(font_id, find_str);

    rect_renderer.addRect(position, size, {69, 75, 84}, Layer::kTwo, 4);

    auto pos = position;
    pos.y += 8;
    pos.x += 10;  // Left padding.
    pos.x += 2;   // Caret width.
    text_renderer.renderLineLayout(
        layout, pos, Layer::kTwo, [](size_t) { return kTextColor; }, 0, size.width);
}

void TextInputWidget::updateMaxScroll() {
    ;
}

}  // namespace gui
