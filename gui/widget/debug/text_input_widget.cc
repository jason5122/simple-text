#include "text_input_widget.h"

#include "gui/renderer/renderer.h"

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
    auto& text_renderer = Renderer::instance().getTextRenderer();

    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    const auto& layout = line_layout_cache.get(font_id, find_str);

    rect_renderer.addRect(position, size, kBackgroundColor, Layer::kTwo, 4);

    auto pos = position;
    pos.y += top_padding;
    pos.x += left_padding;
    pos.x += 2;  // Caret width.
    text_renderer.renderLineLayout(
        layout, pos, Layer::kTwo, [](size_t) { return kTextColor; }, 0, size.width);
}

void TextInputWidget::updateMaxScroll() {
    ;
}

app::CursorStyle TextInputWidget::cursorStyle() const {
    return app::CursorStyle::kIBeam;
}

}  // namespace gui
