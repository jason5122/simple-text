#include "vertical_layout_widget.h"

namespace gui {

VerticalLayoutWidget::VerticalLayoutWidget(const renderer::Size& size) : ContainerWidget{size} {}

void VerticalLayoutWidget::draw(const renderer::Size& screen_size) {
    renderer::Size new_screen_size = screen_size;
    for (auto& child : children) {
        child->draw(new_screen_size);
        new_screen_size.height -= child->getSize().height;
    }
}

void VerticalLayoutWidget::scroll(const renderer::Point& delta) {
    for (auto& child : children) {
        child->scroll(delta);
    }
}

void VerticalLayoutWidget::leftMouseDown(const renderer::Point& mouse) {
    for (auto& child : children) {
        child->leftMouseDown(mouse);
    }
}

void VerticalLayoutWidget::leftMouseDrag(const renderer::Point& mouse) {
    for (auto& child : children) {
        child->leftMouseDrag(mouse);
    }
}

void VerticalLayoutWidget::setPosition(const renderer::Point& position) {
    this->position = position;

    // Recursively update position of children.
    renderer::Point new_position = position;
    for (auto& child : children) {
        child->setPosition(new_position);
        new_position.y += child->getSize().height;
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
