#include "vertical_layout_widget.h"

namespace gui {

VerticalLayoutWidget::VerticalLayoutWidget(const renderer::Size& size) : ContainerWidget{size} {}

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

void VerticalLayoutWidget::leftMouseDown(const renderer::Point& mouse,
                                         const renderer::Point& offset) {
    renderer::Point new_offset = offset;

    for (auto& child : children) {
        child->leftMouseDown(mouse, new_offset);

        new_offset.y += child->getSize().height;
    }
}

void VerticalLayoutWidget::leftMouseDrag(const renderer::Point& mouse,
                                         const renderer::Point& offset) {
    renderer::Point new_offset = offset;

    for (auto& child : children) {
        child->leftMouseDrag(mouse, new_offset);

        new_offset.y += child->getSize().height;
    }
}

void VerticalLayoutWidget::setPosition(const renderer::Point& position) {
    this->position = position;

    // Recursively update position of children.
    renderer::Point new_position = position;
    for (auto& child : children) {
        new_position.y += child->getSize().height;
        child->setPosition(new_position);
    }
}

void VerticalLayoutWidget::addChild(std::unique_ptr<Widget> widget) {
    renderer::Point new_position{};
    for (auto& child : children) {
        new_position.y += child->getSize().height;
        // child->setPosition(new_position);
    }

    widget->setPosition(new_position);
    children.push_back(std::move(widget));

    // this->size.height = new_position.y;
}

}
