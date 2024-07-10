#include "label_widget.h"
#include "renderer/renderer.h"

namespace gui {

void LabelWidget::setText(const std::string& str8) {
    label_text.setText(str8);
}

void LabelWidget::addLeftIcon(size_t icon_id) {
    left_side_icons.emplace_back(icon_id);
}

void LabelWidget::addRightIcon(size_t icon_id) {
    right_side_icons.emplace_back(icon_id);
}

void LabelWidget::draw() {
    renderer::TextRenderer& text_renderer = renderer::Renderer::instance().getTextRenderer();
    renderer::RectRenderer& rect_renderer = renderer::Renderer::instance().getRectRenderer();
    renderer::ImageRenderer& image_renderer = renderer::Renderer::instance().getImageRenderer();

    constexpr renderer::Rgba temp_color{223, 227, 230, 255};
    constexpr renderer::Rgba folder_icon_color{142, 142, 142, 255};

    // rect_renderer.addRect(position, size, temp_color);

    // Draw all left side icons.
    renderer::Point left_offset{};
    for (size_t icon_id : left_side_icons) {
        renderer::Size image_size = image_renderer.getImageSize(icon_id);

        renderer::Point icon_position = centerVertically(image_size.height) + left_offset;
        image_renderer.addImage(icon_id, icon_position, folder_icon_color);

        left_offset.x += image_size.width;
    }

    // Draw all right side icons.
    renderer::Point right_offset{};
    for (size_t icon_id : right_side_icons) {
        renderer::Size image_size = image_renderer.getImageSize(icon_id);

        right_offset.x += image_size.width;

        renderer::Point icon_position = centerVertically(image_size.height) - right_offset;
        icon_position += renderer::Point{size.width, 0};
        image_renderer.addImage(icon_id, icon_position, folder_icon_color);
    }

    renderer::Point text_position = centerVertically(text_renderer.uiLineHeight()) + left_offset;
    text_renderer.addUiText(text_position, label_text);
}

renderer::Point LabelWidget::centerVertically(int widget_height) {
    renderer::Point centered_point = position;
    centered_point.y += size.height / 2;
    centered_point.y -= widget_height / 2;
    return centered_point;
}

}
