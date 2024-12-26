#include "image_button_widget.h"

#include "gui/renderer/renderer.h"

namespace gui {

ImageButtonWidget::ImageButtonWidget(size_t image_id, Rgba bg_color, int padding)
    : image_id(image_id), bg_color(bg_color), padding(padding) {
    auto& image_renderer = Renderer::instance().getImageRenderer();
    auto& image = image_renderer.get(image_id);

    size = image.size;
    size += {padding * 2, padding * 2};
}

void ImageButtonWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    auto& image_renderer = Renderer::instance().getImageRenderer();

    rect_renderer.addRect(position, size, bg_color, Layer::kTwo, 4);

    auto pos = position;
    pos += {padding, padding};
    image_renderer.insertInBatch(image_id, pos, {0, 0, 255, false}, Layer::kTwo);
}

}  // namespace gui
