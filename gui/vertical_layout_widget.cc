#include "vertical_layout_widget.h"

#include <iostream>

namespace gui {

VerticalLayoutWidget::VerticalLayoutWidget(const renderer::Size& size) : ContainerWidget{size} {}

// TODO: Debug use; remove this.
void VerticalLayoutWidget::draw() {
    std::cerr << "VerticalLayout: position = " << position << ", size = " << size << '\n';
    ContainerWidget::draw();
}

void VerticalLayoutWidget::layout() {
    int top_offset = position.y;
    int bottom_offset = position.y + size.height;

    for (auto& child : children_start) {
        child->setPosition({position.x, top_offset});
        child->setWidth(size.width);

        // Recursively layout children.
        child->layout();

        top_offset += child->getSize().height;
    }

    for (auto& child : children_end) {
        bottom_offset -= child->getSize().height;

        child->setPosition({position.x, bottom_offset});
        child->setWidth(size.width);

        // Recursively layout children.
        child->layout();
    }

    if (main_widget) {
        main_widget->setPosition({position.x, top_offset});
        main_widget->setWidth(size.width);
        main_widget->setHeight(bottom_offset - top_offset);

        // Recursively layout main widget.
        main_widget->layout();
    }
}

}
