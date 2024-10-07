#include "vertical_layout_widget.h"

namespace gui {

VerticalLayoutWidget::VerticalLayoutWidget(const Size& size) : LayoutWidget{size} {}

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
        main_widget->setHeight(std::max(bottom_offset - top_offset, 0));

        // Recursively layout main widget.
        main_widget->layout();
    }
}

}
