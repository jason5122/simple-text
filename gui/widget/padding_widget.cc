#include "padding_widget.h"

#include "gui/renderer/renderer.h"

namespace gui {

PaddingWidget::PaddingWidget(const Size& size, const Rgb& color) : Widget{size}, color{color} {}

void PaddingWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position(), size(), position(), position() + size(), color,
                          Layer::kBackground);
}

}  // namespace gui
