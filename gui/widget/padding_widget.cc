#include "gui/renderer/renderer.h"
#include "padding_widget.h"

namespace gui {

PaddingWidget::PaddingWidget(const Size& size, const Rgba& color) : Widget{size}, color{color} {}

void PaddingWidget::draw(const Point& mouse_pos) {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position, size, color, RectRenderer::RectLayer::kForeground);
}

}
