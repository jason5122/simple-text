#include "horizontal_layout_widget.h"

#include <iostream>

namespace gui {

HorizontalLayoutWidget::HorizontalLayoutWidget(const renderer::Size& size)
    : ContainerWidget{size} {}

void HorizontalLayoutWidget::draw(const renderer::Size& screen_size,
                                  const renderer::Point& offset) {
    std::cerr << "HorizontalLayout: position = " << position << ", size = " << size << '\n';

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

void HorizontalLayoutWidget::leftMouseDown(const renderer::Point& mouse,
                                           const renderer::Point& offset) {
    renderer::Point new_offset = offset;

    for (auto& child : children) {
        child->leftMouseDown(mouse, new_offset);

        new_offset.x += child->getSize().width;
    }
}

void HorizontalLayoutWidget::leftMouseDrag(const renderer::Point& mouse,
                                           const renderer::Point& offset) {
    renderer::Point new_offset = offset;

    for (auto& child : children) {
        child->leftMouseDrag(mouse, new_offset);

        new_offset.x += child->getSize().width;
    }
}

void HorizontalLayoutWidget::setPosition(const renderer::Point& position) {
    this->position = position;

    // Recursively update position of children.
    renderer::Point new_position = position;
    for (auto& child : children) {
        new_position.x += child->getSize().width;
        child->setPosition(new_position);
    }
}

void HorizontalLayoutWidget::addChild(std::unique_ptr<Widget> widget) {
    renderer::Point new_position{};
    for (auto& child : children) {
        new_position.x += child->getSize().width;
        // child->setPosition(new_position);
    }

    widget->setPosition(new_position);
    children.push_back(std::move(widget));

    // this->size.width = new_position.x;
}

}
