#include "renderer/renderer.h"
#include "side_bar_widget.h"

namespace gui {

SideBarWidget::SideBarWidget(const renderer::Size& size) : Widget{size} {}

void SideBarWidget::draw() {
    renderer::RectRenderer& rect_renderer = renderer::g_renderer->getRectRenderer();
    rect_renderer.addRect(position, size, {235, 237, 239, 255});
}

}
