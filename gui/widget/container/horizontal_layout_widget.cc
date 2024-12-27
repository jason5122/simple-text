#include "horizontal_layout_widget.h"

namespace gui {

HorizontalLayoutWidget::HorizontalLayoutWidget(int padding_in_between,
                                               int left_padding,
                                               int right_padding,
                                               int top_padding)
    : padding_in_between(padding_in_between),
      left_padding(left_padding),
      right_padding(right_padding),
      top_padding(top_padding) {}

void HorizontalLayoutWidget::layout() {
    int left_offset = position.x;
    int right_offset = position.x + size.width;
    int top_offset = position.y;

    left_offset += left_padding;
    right_offset -= right_padding;
    top_offset += top_padding;

    for (auto& child : children_start) {
        left_offset += padding_in_between;

        child->setPosition({left_offset, top_offset});
        if (child->isAutoresizing()) {
            child->setHeight(size.height);
        }

        // Recursively layout children.
        child->layout();

        left_offset += child->getSize().width;
    }

    for (auto& child : children_end) {
        right_offset -= padding_in_between;

        right_offset -= child->getSize().width;

        child->setPosition({right_offset, top_offset});
        if (child->isAutoresizing()) {
            child->setHeight(size.height);
        }

        // Recursively layout children.
        child->layout();
    }

    if (main_widget) {
        // Add padding between main widget and children.
        left_offset += padding_in_between;
        right_offset -= padding_in_between;

        main_widget->setPosition({left_offset, top_offset});
        main_widget->setWidth(std::max(right_offset - left_offset, 0));
        if (main_widget->isAutoresizing()) {
            main_widget->setHeight(size.height);
        }

        // Recursively layout main widget.
        main_widget->layout();
    }
}

}  // namespace gui
