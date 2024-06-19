#include "side_bar_widget.h"

namespace gui {

SideBarWidget::SideBarWidget(std::shared_ptr<renderer::Renderer> renderer, int side_bar_width)
    : Widget{renderer}, side_bar_width{side_bar_width} {}

void SideBarWidget::draw(int width, int height) {
    renderer->getRectRenderer().addRect({0, 0}, {side_bar_width, height}, {235, 237, 239, 255});
}

void SideBarWidget::scroll(int dx, int dy) {}

}
