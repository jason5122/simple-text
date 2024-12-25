#include "button_widget.h"

#include "gui/renderer/renderer.h"

namespace gui {

ButtonWidget::ButtonWidget(size_t image_id, Rgba bg_color)
    : image_id(image_id), bg_color(bg_color) {
    auto& image_renderer = Renderer::instance().getImageRenderer();
    auto& image = image_renderer.get(image_id);

    setSize(image.size);
}

void ButtonWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    auto& image_renderer = Renderer::instance().getImageRenderer();

    rect_renderer.addRect(position, size, bg_color, Layer::kTwo, 4);
    image_renderer.insertInBatch(image_id, position, {0, 0, 255, false}, Layer::kTwo);
}

}  // namespace gui
