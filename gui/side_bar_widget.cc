#include "renderer/renderer.h"
#include "side_bar_widget.h"

#include <iostream>

namespace gui {

SideBarWidget::SideBarWidget(const renderer::Size& size) : Widget{size} {}

void SideBarWidget::draw(const renderer::Size& screen_size) {
    std::cerr << "SideBarWidget: position = " << position << ", size = " << size << '\n';

    renderer::g_renderer->getRectRenderer().addRect(position, {size.width, screen_size.height},
                                                    {235, 237, 239, 255});
}

}
