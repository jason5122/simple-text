#include "horizontal_layout_widget.h"

#include <iostream>

namespace gui {

HorizontalLayoutWidget::HorizontalLayoutWidget(const renderer::Size& size)
    : ContainerWidget{size} {}

void HorizontalLayoutWidget::draw(const renderer::Size& screen_size) {
    std::cerr << "HorizontalLayout: position = " << position << ", size = " << size << '\n';

    if (main_widget) {
        main_widget->draw(screen_size);
    }

    renderer::Size new_screen_size = screen_size;
    for (auto& child : children) {
        child->draw(new_screen_size);
        new_screen_size.width -= child->getSize().width;
    }
}

void HorizontalLayoutWidget::setPosition(const renderer::Point& position) {
    this->position = position;

    // Recursively update position of children.
    renderer::Point new_position = position;
    for (auto& child : children) {
        child->setPosition(new_position);
        new_position.x += child->getSize().width;
    }
}

void HorizontalLayoutWidget::setMainWidget(std::unique_ptr<Widget> widget) {
    main_widget = std::move(widget);
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
