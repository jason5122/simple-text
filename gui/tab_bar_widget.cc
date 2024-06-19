#include "tab_bar_widget.h"

namespace gui {

TabBarWidget::TabBarWidget(std::shared_ptr<renderer::Renderer> renderer, int tab_bar_height)
    : Widget{renderer}, tab_bar_height{tab_bar_height} {}

void TabBarWidget::draw(int width, int height) {
    renderer->getRectRenderer().addRect({0, 0}, {width, tab_bar_height}, {190, 190, 190, 255});
}

void TabBarWidget::scroll(int dx, int dy) {}

}
