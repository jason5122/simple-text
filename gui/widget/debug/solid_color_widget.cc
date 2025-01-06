#include "solid_color_widget.h"

#include "gui/renderer/renderer.h"

namespace gui {

SolidColorWidget::SolidColorWidget(const app::Size& size, const Rgba& color)
    : Widget(size), color(color) {}

void SolidColorWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position, size, position, position + size, color, Layer::kForeground);
}

}  // namespace gui
