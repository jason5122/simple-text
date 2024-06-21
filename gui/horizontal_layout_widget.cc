#include "horizontal_layout_widget.h"

namespace gui {

HorizontalLayoutWidget::HorizontalLayoutWidget(const renderer::Size& size)
    : ContainerWidget{size} {}

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

void HorizontalLayoutWidget::leftMouseDown(const renderer::Point& mouse) {
    for (auto& child : children) {
        child->leftMouseDown(mouse);
    }
}

void HorizontalLayoutWidget::addChild(std::unique_ptr<Widget> widget) {
    children.push_back(std::move(widget));
}

}
