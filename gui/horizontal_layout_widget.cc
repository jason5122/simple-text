#include "horizontal_layout_widget.h"

namespace gui {

HorizontalLayoutWidget::HorizontalLayoutWidget(std::shared_ptr<renderer::Renderer> renderer,
                                               const renderer::Size& size)
    : Widget{renderer, size} {}

void HorizontalLayoutWidget::draw(const renderer::Size& screen_size,
                                  const renderer::Point& offset) {
    renderer::Size new_screen_size = screen_size;
    renderer::Point new_offset = offset;

    for (auto& child : children) {
        child->draw(new_screen_size, new_offset);

        int child_width = child->getSize().width;
        new_screen_size.width -= child_width;
        new_offset.x += child_width;
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
