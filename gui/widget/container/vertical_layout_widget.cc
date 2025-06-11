#include "gui/widget/container/vertical_layout_widget.h"

namespace gui {

void VerticalLayoutWidget::layout() {
    int top_offset = position().y;
    int bottom_offset = position().y + height();

    for (auto& child : children_start) {
        child->set_position({position().x, top_offset});
        if (child->is_autoresizing()) {
            child->set_width(width());
        }

        // Recursively layout children.
        child->layout();

        top_offset += child->height();
    }

    for (auto& child : children_end) {
        bottom_offset -= child->height();

        child->set_position({position().x, bottom_offset});
        if (child->is_autoresizing()) {
            child->set_width(width());
        }

        // Recursively layout children.
        child->layout();
    }

    if (main_widget) {
        main_widget->set_position({position().x, top_offset});
        main_widget->set_height(std::max(bottom_offset - top_offset, 0));
        if (main_widget->is_autoresizing()) {
            main_widget->set_width(width());
        }

        // Recursively layout main widget.
        main_widget->layout();
    }
}

}  // namespace gui
