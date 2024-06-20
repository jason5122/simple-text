#include "vertical_layout_widget.h"

namespace gui {

VerticalLayoutWidget::VerticalLayoutWidget(std::shared_ptr<renderer::Renderer> renderer,
                                           const renderer::Size& size)
    : Widget{renderer, size} {}

void VerticalLayoutWidget::draw(const renderer::Size& screen_size, const renderer::Point& offset) {
    renderer::Size new_screen_size = screen_size;
    renderer::Point new_offset = offset;

    for (auto& child : children) {
        child->draw(new_screen_size, new_offset);

        int child_height = child->getSize().height;
        new_screen_size.height -= child_height;
        new_offset.y += child_height;
    }
}

void VerticalLayoutWidget::scroll(const renderer::Point& delta) {
    for (auto& child : children) {
        child->scroll(delta);
    }
}

void VerticalLayoutWidget::addChild(std::unique_ptr<Widget> widget) {
    children.push_back(std::move(widget));
}

}
