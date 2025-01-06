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
    auto& texture_renderer = Renderer::instance().getTextureRenderer();
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();

    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);
    const auto& layout = line_layout_cache.get(font_id, label_str);

    Point coords = centerVertically(metrics.line_height);
    Point min_coords = {
        .x = 0,
        .y = position.y,
    };
    Point max_coords = {
        .x = size.width,
        .y = position.y + size.height,
    };
    const auto highlight_callback = [this](size_t) { return color; };
    texture_renderer.addLineLayout(layout, coords, min_coords, max_coords, highlight_callback);
}

Point LabelWidget::centerVertically(int widget_height) {
    Point centered_point = position;
    centered_point.y += size.height / 2;
    centered_point.y -= widget_height / 2;
    return centered_point;
}

}  // namespace gui
