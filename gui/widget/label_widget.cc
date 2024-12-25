#include "label_widget.h"

#include "gui/renderer/renderer.h"

// TODO: Debug use; remove this.
#include "util/profile_util.h"

namespace gui {

LabelWidget::LabelWidget(size_t font_id,
                         const app::Size& size,
                         int left_padding,
                         int right_padding)
    : Widget{size}, font_id(font_id), left_padding(left_padding), right_padding(right_padding) {}

void LabelWidget::setText(std::string_view str8) {
    this->label_str = str8;
}

void LabelWidget::setColor(const Rgb& color) {
    this->color = color;
}

void LabelWidget::addLeftIcon(size_t icon_id) {
    left_side_icons.emplace_back(icon_id);
}

void LabelWidget::addRightIcon(size_t icon_id) {
    right_side_icons.emplace_back(icon_id);
}

void LabelWidget::draw() {
    auto& text_renderer = Renderer::instance().getTextRenderer();
    auto& image_renderer = Renderer::instance().getImageRenderer();
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();

    // auto& rect_renderer = Renderer::instance().getRectRenderer();
    // rect_renderer.addRect(position, size, {255, 0, 0, 0}, RectRenderer::RectLayer::kForeground);

    // Draw all left side icons.
    app::Point left_offset{.x = left_padding};
    for (size_t icon_id : left_side_icons) {
        auto& image = image_renderer.get(icon_id);

        app::Point icon_position = centerVertically(image.size.height) + left_offset;
        image_renderer.insertInBatch(icon_id, icon_position, kFolderIconColor, Layer::kTwo);

        left_offset.x += image.size.width;
    }

    // Draw all right side icons.
    app::Point right_offset{.x = right_padding};
    for (size_t icon_id : right_side_icons) {
        auto& image = image_renderer.get(icon_id);

        right_offset.x += image.size.width;

        app::Point icon_position = centerVertically(image.size.height) - right_offset;
        icon_position += app::Point{size.width, 0};
        image_renderer.insertInBatch(icon_id, icon_position, kFolderIconColor, Layer::kTwo);
    }

    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);
    const auto& layout = line_layout_cache.get(font_id, label_str);

    app::Point coords = centerVertically(metrics.line_height) + left_offset;
    int min_x = 0;
    int max_x = size.width - left_padding - right_padding;
    const auto highlight_callback = [this](size_t) { return color; };
    text_renderer.renderLineLayout(layout, coords, Layer::kTwo, highlight_callback, min_x, max_x);
}

app::Point LabelWidget::centerVertically(int widget_height) {
    app::Point centered_point = position;
    centered_point.y += size.height / 2;
    centered_point.y -= widget_height / 2;
    return centered_point;
}

}  // namespace gui
