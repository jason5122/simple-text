#include "gui/renderer/renderer.h"
#include "gui/widget/debug/solid_color_widget.h"

namespace gui {

SolidColorWidget::SolidColorWidget(const Size& size, const Rgb& color)
    : Widget(size), color(color) {}

void SolidColorWidget::draw() {
    auto& rect_renderer = Renderer::instance().rect_renderer();
    rect_renderer.add_rect(position(), size(), position(), position() + size(), color,
                           Layer::kForeground);
}

}  // namespace gui
