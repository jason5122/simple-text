#include "renderer/renderer.h"
#include "tab_bar_widget.h"

namespace gui {

TabBarWidget::TabBarWidget(const renderer::Size& size) : Widget{size} {}

void TabBarWidget::draw() {
    renderer::RectRenderer& rect_renderer = renderer::Renderer::instance().getRectRenderer();
    renderer::ImageRenderer& image_renderer = renderer::Renderer::instance().getImageRenderer();

    rect_renderer.addRect(position, size, {190, 190, 190, 255});

    // Leave padding between window title bar and tab.
    renderer::Point padding_top{0, 3 * 2};
    int tab_height = size.height - padding_top.y;
    int tab_corner_radius = 10;
    int tab_width = 360;

    // TODO: Unify `RectRenderer::addTab()` with `RectRenderer::addRect()`.
    // rect_renderer.addTab(position + padding_top, {tab_width, tab_height}, {100, 100, 100, 255},
    //                      tab_corner_radius);

    renderer::Size image_size =
        image_renderer.getImageSize(renderer::ImageRenderer::kPanelClose2xIndex);

    renderer::Point image_pos = position + padding_top;
    image_pos.x += tab_corner_radius;
    image_pos.y += tab_height / 2;
    image_pos.y -= image_size.height / 2;

    image_pos.x += 340 - image_size.width;
    image_pos.x -= 15;  // TODO: Don't hard code this value.

    for (size_t i = 0; i < 3; i++) {
        image_renderer.addImage(renderer::ImageRenderer::kPanelClose2xIndex, image_pos,
                                {142, 142, 142, 255});
        image_pos += renderer::Point{340, 0};
    }

    int tab_index = 2;
    rect_renderer.addTab(position + padding_top + renderer::Point{340 * tab_index, 0},
                         {tab_width, tab_height}, {253, 253, 253, 255}, tab_corner_radius);

    // TODO: Figure out why we need to add 1 to `26 / 2`.
    rect_renderer.addRect(position + renderer::Point{350 * 1 - 2, 26 / 2 + 1},
                          {2, size.height - 26}, {148, 149, 149, 255});
}

}
