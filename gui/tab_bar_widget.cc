#include "tab_bar_widget.h"

namespace gui {

TabBarWidget::TabBarWidget(std::shared_ptr<renderer::Renderer> renderer, int tab_bar_height)
    : Widget{renderer}, tab_bar_height{tab_bar_height} {}

void TabBarWidget::draw(const renderer::Size& size, const renderer::Point& offset) {
    renderer->getRectRenderer().addRect({0, 0}, {size.width, tab_bar_height},
                                        {190, 190, 190, 255});
}

void TabBarWidget::scroll(const renderer::Point& delta) {}

}
