#include "horizontal_layout_widget.h"

#include <iostream>

namespace gui {

HorizontalLayoutWidget::HorizontalLayoutWidget(const renderer::Size& size)
    : ContainerWidget{size} {}

// TODO: Debug use; remove this.
void HorizontalLayoutWidget::draw() {
    std::cerr << "HorizontalLayout: position = " << position << ", size = " << size << '\n';
    ContainerWidget::draw();
}

void HorizontalLayoutWidget::layout() {
    int left_offset = position.x;
    int right_offset = position.x + size.width;

    for (auto& child : children_start) {
        child->setPosition({left_offset, position.y});
        child->setHeight(size.height);

        // Recursively layout children.
        child->layout();

        left_offset += child->getSize().width;
    }

    for (auto& child : children_end) {
        right_offset -= child->getSize().width;

        child->setPosition({right_offset, position.y});
        child->setHeight(size.height);

        // Recursively layout children.
        child->layout();
    }

    if (main_widget) {
        main_widget->setPosition({left_offset, position.y});
        main_widget->setWidth(right_offset - left_offset);
        main_widget->setHeight(size.height);

        // Recursively layout main widget.
        main_widget->layout();
    }
}

}
