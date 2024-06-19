#include "side_bar_widget.h"

namespace gui {

SideBarWidget::SideBarWidget(std::shared_ptr<renderer::Renderer> renderer) : Widget{renderer} {}

void SideBarWidget::draw(int width, int height) {
    renderer->getRectRenderer().addRect({0, 0}, {200 * 2, height}, {235, 237, 239, 255});
}

}
