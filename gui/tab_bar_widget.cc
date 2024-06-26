#include "renderer/renderer.h"
#include "tab_bar_widget.h"

#include <iostream>

namespace gui {

TabBarWidget::TabBarWidget(const renderer::Size& size) : Widget{size} {}

void TabBarWidget::draw() {
    std::cerr << "TabBar: position = " << position << ", size = " << size << '\n';

    renderer::RectRenderer& rect_renderer = renderer::g_renderer->getRectRenderer();
    renderer::ImageRenderer& image_renderer = renderer::g_renderer->getImageRenderer();

    rect_renderer.addRect(position, size, {190, 190, 190, 255});

    // Leave padding between window title bar and tab.
    renderer::Point padding_top{0, 3 * 2};
    int tab_height = size.height - padding_top.y;
    int tab_corner_radius = 10;

    // TODO: Unify `RectRenderer::addTab()` with `RectRenderer::addRect()`.
    rect_renderer.addTab(position + padding_top, {360, tab_height}, {253, 253, 253, 255},
                         tab_corner_radius);

    image_renderer.addImage(position, {158, 158, 158, 255});
}

}
