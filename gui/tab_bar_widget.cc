#include "renderer/renderer.h"
#include "tab_bar_widget.h"

namespace gui {

TabBarWidget::TabBarWidget(const renderer::Size& size) : Widget{size} {}

void TabBarWidget::draw(const renderer::Size& screen_size, const renderer::Point& offset) {
    renderer::g_renderer->getRectRenderer().addRect(offset, {screen_size.width, size.height},
                                                    {190, 190, 190, 255});

    // TODO: Unify `RectRenderer::addTab()` with `RectRenderer::addRect()`.
    int padding_top = 3 * 2;
    int tab_height = size.height - padding_top;  // Leave padding between window title bar and tab.
    int tab_corner_radius = 10;
    renderer::g_renderer->getRectRenderer().addTab({offset.x, offset.y + padding_top},
                                                   {350, tab_height}, {253, 253, 253, 255},
                                                   tab_corner_radius);
}

void TabBarWidget::scroll(const renderer::Point& delta) {}

void TabBarWidget::leftMouseDown(const renderer::Point& mouse) {}

}
