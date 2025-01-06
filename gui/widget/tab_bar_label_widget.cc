#include "tab_bar_label_widget.h"

#include "gui/renderer/renderer.h"

namespace gui {

TabBarLabelWidget::TabBarLabelWidget(size_t font_id,
                                     const app::Size& size,
                                     int left_padding,
                                     int right_padding)
    : Widget{size}, font_id(font_id), left_padding(left_padding), right_padding(right_padding) {}

void TabBarLabelWidget::setText(std::string_view str8) {
    this->label_str = str8;
}

void TabBarLabelWidget::setColor(const Rgb& color) {
    this->color = color;
}

void TabBarLabelWidget::addLeftIcon(size_t icon_id) {
    left_side_icons.emplace_back(icon_id);
}

void TabBarLabelWidget::addRightIcon(size_t icon_id) {
    right_side_icons.emplace_back(icon_id);
}

void TabBarLabelWidget::draw() {
    auto& texture_renderer = Renderer::instance().getTextureRenderer();
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    const auto& texture_cache = gui::Renderer::instance().getTextureCache();

    // Draw all left side icons.
    app::Point left_offset{.x = left_padding};
    for (size_t icon_id : left_side_icons) {
        auto& image = texture_cache.getImage(icon_id);

        app::Point icon_position = centerVertically(image.size.height) + left_offset;
        texture_renderer.addImage(icon_id, icon_position, kFolderIconColor);

        left_offset.x += image.size.width;
    }

    // Draw all right side icons.
    app::Point right_offset{.x = right_padding};
    for (size_t icon_id : right_side_icons) {
        auto& image = texture_cache.getImage(icon_id);

        right_offset.x += image.size.width;

        app::Point icon_position = centerVertically(image.size.height) - right_offset;
        icon_position += app::Point{size.width, 0};
        texture_renderer.addImage(icon_id, icon_position, kFolderIconColor);
    }

    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);
    const auto& layout = line_layout_cache.get(font_id, label_str);

    app::Point coords = centerVertically(metrics.line_height) + left_offset;
    app::Point min_coords = {
        .x = 0,
        .y = position.y,
    };
    app::Point max_coords = {
        .x = size.width - left_padding - right_padding,
        .y = position.y + size.height,
    };
    const auto highlight_callback = [this](size_t) { return color; };
    texture_renderer.addLineLayout(layout, coords, min_coords, max_coords, highlight_callback);
}

app::Point TabBarLabelWidget::centerVertically(int widget_height) {
    app::Point centered_point = position;
    centered_point.y += size.height / 2;
    centered_point.y -= widget_height / 2;
    return centered_point;
}

}  // namespace gui
