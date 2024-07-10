#include "gui/renderer/renderer.h"
#include "padding_widget.h"

namespace gui {

PaddingWidget::PaddingWidget(const renderer::Size& size, const renderer::Rgba& color)
    : Widget{size}, color{color} {}

void PaddingWidget::draw() {
    renderer::RectRenderer& rect_renderer = renderer::Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position, size, color);
}

}
