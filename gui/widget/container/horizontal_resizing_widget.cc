#include "gui/widget/container/horizontal_resizing_widget.h"

namespace gui {

HorizontalResizingWidget::HorizontalResizingWidget(int padding_in_between,
                                                   int left_padding,
                                                   int right_padding,
                                                   int top_padding)
    : HorizontalLayoutWidget(padding_in_between, left_padding, right_padding, top_padding) {}

void HorizontalResizingWidget::left_mouse_down(const Point& mouse_pos,
                                               ModifierKey modifiers,
                                               ClickType click_type) {
    for (auto& child : children_start) {
        if (child->right_edge_test(mouse_pos, kResizeDistance)) {
            dragged_widget = child.get();
            is_dragged_widget_start = true;
            return;
        }
    }
    for (auto& child : children_end) {
        if (child->left_edge_test(mouse_pos, kResizeDistance)) {
            dragged_widget = child.get();
            is_dragged_widget_start = false;
            return;
        }
    }
}

void HorizontalResizingWidget::left_mouse_drag(const Point& mouse_pos,
                                               ModifierKey modifiers,
                                               ClickType click_type) {
    if (dragged_widget) {
        auto widget_size = dragged_widget->size();
        auto widget_pos = dragged_widget->position();

        int new_width;
        if (is_dragged_widget_start) {
            new_width = mouse_pos.x - widget_pos.x;
        } else {
            new_width = (widget_pos.x + widget_size.width) - mouse_pos.x;
        }
        dragged_widget->set_width(new_width);
    }
}

CursorStyle HorizontalResizingWidget::cursor_style() const {
    return CursorStyle::kResizeLeftRight;
}

Widget* HorizontalResizingWidget::widget_at(const Point& pos) {
    // If mouse cursor is over a resizable widget edge, return this widget for resizing purposes.
    for (auto& child : children_start) {
        if (child->is_resizable() && child->right_edge_test(pos, kResizeDistance)) {
            return this;
        }
    }
    for (auto& child : children_end) {
        if (child->is_resizable() && child->left_edge_test(pos, kResizeDistance)) {
            return this;
        }
    }

    // Otherwise, propagate as normal.
    return LayoutWidget::widget_at(pos);
}

}  // namespace gui
