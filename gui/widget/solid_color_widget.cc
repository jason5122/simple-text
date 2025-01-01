#include "solid_color_widget.h"

#include "gui/renderer/renderer_lite.h"

namespace gui {

SolidColorWidget::SolidColorWidget(const app::Size& size, const Rgba& color)
    : Widget(size), color(color) {}

void SolidColorWidget::draw() {
    auto& rect_renderer = RendererLite::instance().getRectRenderer();
    rect_renderer.addRect(position, size, color, Layer::kForeground);
}

}  // namespace gui
