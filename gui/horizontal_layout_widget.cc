#include "horizontal_layout_widget.h"

namespace gui {

HorizontalLayoutWidget::HorizontalLayoutWidget(std::shared_ptr<renderer::Renderer> renderer,
                                               const renderer::Size& size)
    : Widget{renderer, size} {}

void HorizontalLayoutWidget::draw(const renderer::Size& screen_size,
                                  const renderer::Point& offset) {
    for (auto& child : children) {
        child->draw(screen_size, offset);
    }
}

void HorizontalLayoutWidget::scroll(const renderer::Point& delta) {
    for (auto& child : children) {
        child->scroll(delta);
    }
}

void HorizontalLayoutWidget::addChild(std::unique_ptr<Widget> widget) {
    children.push_back(std::move(widget));
}

}
