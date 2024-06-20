#include "tab_bar_widget.h"

namespace gui {

TabBarWidget::TabBarWidget(std::shared_ptr<renderer::Renderer> renderer,
                           const renderer::Size& size)
    : Widget{renderer, size} {}

void TabBarWidget::draw(const renderer::Size& screen_size, const renderer::Point& offset) {
    renderer->getRectRenderer().addRect({0, 0}, {screen_size.width, size.height},
                                        {190, 190, 190, 255});
}

void TabBarWidget::scroll(const renderer::Point& delta) {}

}
