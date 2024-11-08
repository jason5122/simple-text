#include "padding_widget.h"

#include "gui/renderer/renderer.h"

namespace gui {

PaddingWidget::PaddingWidget(const app::Size& size, const Rgba& color)
    : Widget{size}, color{color} {}

void PaddingWidget::draw(const std::optional<app::Point>& mouse_pos) {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position, size, color, RectRenderer::RectLayer::kForeground);
}

}  // namespace gui
