#include "label_widget.h"

#include "gui/renderer/renderer.h"

namespace gui {

LabelWidget::LabelWidget(size_t font_id, const Rgb& color, int left_padding, int right_padding)
    : font_id(font_id), color(color) {
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);

    setHeight(metrics.line_height);
}

void LabelWidget::setText(std::string_view str8) {
    this->label_str = str8;

    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    const auto& layout = line_layout_cache.get(font_id, label_str);
    setWidth(layout.width);
}

void LabelWidget::draw() {
    auto& text_renderer = Renderer::instance().getTextRenderer();
    auto& image_renderer = Renderer::instance().getImageRenderer();
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();

    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);
    const auto& layout = line_layout_cache.get(font_id, label_str);

    app::Point coords = centerVertically(metrics.line_height);
    int min_x = 0;
    int max_x = size.width;
    const auto highlight_callback = [this](size_t) { return color; };
    text_renderer.renderLineLayout(layout, coords, highlight_callback, min_x, max_x);
}

app::Point LabelWidget::centerVertically(int widget_height) {
    app::Point centered_point = position;
    centered_point.y += size.height / 2;
    centered_point.y -= widget_height / 2;
    return centered_point;
}

}  // namespace gui
