#include "renderer/renderer.h"
#include "side_bar_widget.h"

namespace gui {

SideBarWidget::SideBarWidget(const renderer::Size& size) : Widget{size} {}

void SideBarWidget::draw(const renderer::Size& screen_size, const renderer::Point& offset) {
    renderer::g_renderer->getRectRenderer().addRect(offset, {size.width, screen_size.height},
                                                    {235, 237, 239, 255});
}

void SideBarWidget::scroll(const renderer::Point& delta) {}

void SideBarWidget::leftMouseDown(const renderer::Point& mouse, const renderer::Point& offset) {}

void SideBarWidget::leftMouseDrag(const renderer::Point& mouse, const renderer::Point& offset) {}

}
