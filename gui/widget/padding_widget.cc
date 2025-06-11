#include "gui/renderer/renderer.h"
#include "gui/widget/padding_widget.h"

namespace gui {

PaddingWidget::PaddingWidget(const Size& size, const Rgb& color) : Widget{size}, color{color} {}

void PaddingWidget::draw() {
    auto& rect_renderer = Renderer::instance().rect_renderer();
    rect_renderer.add_rect(position(), size(), position(), position() + size(), color,
                           Layer::kBackground);
}

}  // namespace gui
