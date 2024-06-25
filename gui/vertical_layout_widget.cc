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
    int top_offset = 0;
    for (auto& child : children) {
        child->setPosition({position.x, top_offset});
        top_offset += child->getSize().height;

        // Recursively layout children.
        child->layout();
    }

    if (main_widget) {
        main_widget->setPosition({position.x, top_offset});
        main_widget->setWidth(size.width);
        main_widget->setHeight(size.height - top_offset);

        // Recursively layout main widget.
        main_widget->layout();
    }
}

}
