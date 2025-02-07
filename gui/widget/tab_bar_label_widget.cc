#include "tab_bar_label_widget.h"

#include "gui/renderer/renderer.h"

namespace gui {

TabBarLabelWidget::TabBarLabelWidget(size_t font_id,
                                     const Size& size,
                                     int left_padding,
                                     int right_padding)
    : Widget{size}, font_id(font_id), left_padding(left_padding), right_padding(right_padding) {}

void TabBarLabelWidget::set_text(std::string_view str8) {
    this->label_str = str8;
}

void TabBarLabelWidget::set_color(const Rgb& color) {
    this->color = color;
}

void TabBarLabelWidget::add_left_icon(size_t icon_id) {
    left_side_icons.emplace_back(icon_id);
}

void TabBarLabelWidget::add_right_icon(size_t icon_id) {
    right_side_icons.emplace_back(icon_id);
}

void TabBarLabelWidget::draw() {
    auto& texture_renderer = Renderer::instance().getTextureRenderer();
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    const auto& texture_cache = gui::Renderer::instance().getTextureCache();

    // Draw all left side icons.
    Point left_offset{.x = left_padding};
    for (size_t icon_id : left_side_icons) {
        auto& image = texture_cache.get_image(icon_id);

        Point icon_position = center_vertically(image.size.height) + left_offset;
        texture_renderer.addImage(icon_id, icon_position, kFolderIconColor);

        left_offset.x += image.size.width;
    }

    // Draw all right side icons.
    Point right_offset{.x = right_padding};
    for (size_t icon_id : right_side_icons) {
        auto& image = texture_cache.get_image(icon_id);

        right_offset.x += image.size.width;

        Point icon_position = center_vertically(image.size.height) - right_offset;
        icon_position += Point{width(), 0};
        texture_renderer.addImage(icon_id, icon_position, kFolderIconColor);
    }

    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);
    const auto& layout = line_layout_cache.get(font_id, label_str);

    Point coords = center_vertically(metrics.line_height) + left_offset;
    Point min_coords = {
        .x = 0,
        .y = position().y,
    };
    Point max_coords = {
        .x = width() - left_padding - right_padding,
        .y = position().y + height(),
    };
    const auto highlight_callback = [this](size_t) { return color; };
    texture_renderer.addLineLayout(layout, coords, min_coords, max_coords, highlight_callback);
}

Point TabBarLabelWidget::center_vertically(int widget_height) {
    Point centered_point = position();
    centered_point.y += height() / 2;
    centered_point.y -= widget_height / 2;
    return centered_point;
}

}  // namespace gui
