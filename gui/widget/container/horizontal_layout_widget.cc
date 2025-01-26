#include "horizontal_layout_widget.h"

namespace gui {

HorizontalLayoutWidget::HorizontalLayoutWidget(int padding_in_between,
                                               int left_padding,
                                               int right_padding,
                                               int top_padding,
                                               int bottom_padding)
    : padding_in_between(padding_in_between),
      left_padding(left_padding),
      right_padding(right_padding),
      top_padding(top_padding),
      bottom_padding(bottom_padding) {}

void HorizontalLayoutWidget::layout() {
    int left_offset = position().x;
    int right_offset = position().x + width();
    int top_offset = position().y;
    int bottom_offset = position().y + height();

    left_offset += left_padding;
    right_offset -= right_padding;
    top_offset += top_padding;
    bottom_offset -= bottom_padding;

    int new_height = std::max(bottom_offset - top_offset, 0);

    for (auto& child : children_start) {
        left_offset += padding_in_between;

        child->set_position({left_offset, top_offset});
        if (child->is_autoresizing()) {
            child->set_height(new_height);
        }

        // Recursively layout children.
        child->layout();

        left_offset += child->width();
    }

    for (auto& child : children_end) {
        right_offset -= padding_in_between;

        right_offset -= child->width();

        child->set_position({right_offset, top_offset});
        if (child->is_autoresizing()) {
            child->set_height(new_height);
        }

        // Recursively layout children.
        child->layout();
    }

    if (main_widget) {
        // Add padding between main widget and children.
        left_offset += padding_in_between;
        right_offset -= padding_in_between;

        main_widget->set_position({left_offset, top_offset});
        main_widget->set_width(std::max(right_offset - left_offset, 0));
        if (main_widget->is_autoresizing()) {
            main_widget->set_height(new_height);
        }

        // Recursively layout main widget.
        main_widget->layout();
    }
}

}  // namespace gui
