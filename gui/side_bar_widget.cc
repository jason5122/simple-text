#include "side_bar_widget.h"

namespace gui {

SideBarWidget::SideBarWidget(std::shared_ptr<renderer::Renderer> renderer,
                             const renderer::Size& size)
    : Widget{renderer, size} {}

void SideBarWidget::draw(const renderer::Size& screen_size, const renderer::Point& offset) {
    renderer->getRectRenderer().addRect({0, 0}, {size.width, screen_size.height},
                                        {235, 237, 239, 255});
}

void SideBarWidget::scroll(const renderer::Point& delta) {}

}
