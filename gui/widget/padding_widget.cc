#include "padding_widget.h"

#include "gui/renderer/renderer.h"

namespace gui {

PaddingWidget::PaddingWidget(const app::Size& size, const Rgba& color)
    : Widget{size}, color{color} {}

void PaddingWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position, size, color, Layer::kForeground);
}

}  // namespace gui
