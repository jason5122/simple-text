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
    int tab_width = 360;

    // TODO: Unify `RectRenderer::addTab()` with `RectRenderer::addRect()`.
    rect_renderer.addTab(position + padding_top, {tab_width, tab_height}, {253, 253, 253, 255},
                         tab_corner_radius);

    renderer::Size image_size =
        image_renderer.getImageSize(renderer::ImageRenderer::kPanelCloseImageIndex);

    renderer::Point image_pos = position + padding_top;
    image_pos.x += tab_corner_radius;
    image_pos.y += tab_height / 2;
    image_pos.y -= image_size.height / 2;

    image_pos.x += tab_width - image_size.width - tab_corner_radius * 2;
    image_pos.x -= 15;  // TODO: Don't hard code this value.
    image_renderer.addImage(renderer::ImageRenderer::kPanelCloseImageIndex, image_pos,
                            {158, 158, 158, 255});
}

}
