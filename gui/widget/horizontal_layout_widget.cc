#include "horizontal_layout_widget.h"

namespace gui {

HorizontalLayoutWidget::HorizontalLayoutWidget(const Size& size) : ContainerWidget{size} {}

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
        main_widget->setWidth(std::max(right_offset - left_offset, 0));
        main_widget->setHeight(size.height);

        // Recursively layout main widget.
        main_widget->layout();
    }
}

}
