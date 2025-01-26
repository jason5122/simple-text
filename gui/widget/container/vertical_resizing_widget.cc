#include "vertical_resizing_widget.h"

#include <fmt/base.h>

namespace gui {

void VerticalResizingWidget::left_mouse_down(const Point& mouse_pos,
                                           ModifierKey modifiers,
                                           ClickType click_type) {
    for (auto& child : children_start) {
        if (child->bottom_edge_test(mouse_pos, kResizeDistance)) {
            if (click_type == ClickType::kDoubleClick) {
                child->set_height(child->min_height());
            } else {
                dragged_widget = child.get();
                is_dragged_widget_start = true;
                return;
            }
        }
    }
    for (auto& child : children_end) {
        if (child->top_edge_test(mouse_pos, kResizeDistance)) {
            if (click_type == ClickType::kDoubleClick) {
                child->set_height(child->min_height());
            } else {
                dragged_widget = child.get();
                is_dragged_widget_start = false;
                return;
            }
        }
    }
}

void VerticalResizingWidget::left_mouse_drag(const Point& mouse_pos,
                                           ModifierKey modifiers,
                                           ClickType click_type) {
    // TODO: If a double click occurred, prevent the drag from starting in the first place.
    if (click_type == ClickType::kDoubleClick) {
        return;
    }

    if (dragged_widget) {
        auto widget_size = dragged_widget->size();
        auto widget_pos = dragged_widget->position();

        int new_height;
        if (is_dragged_widget_start) {
            new_height = mouse_pos.y - widget_pos.y;
        } else {
            new_height = (widget_pos.y + widget_size.height) - mouse_pos.y;
        }
        dragged_widget->set_height(new_height);
    }
}

CursorStyle VerticalResizingWidget::cursor_style() const {
    return CursorStyle::kResizeUpDown;
}

Widget* VerticalResizingWidget::widget_at(const Point& pos) {
    // If mouse cursor is over a resizable widget edge, return this widget for resizing purposes.
    for (auto& child : children_start) {
        if (child->is_resizable() && child->bottom_edge_test(pos, kResizeDistance)) {
            return this;
        }
    }
    for (auto& child : children_end) {
        if (child->is_resizable() && child->top_edge_test(pos, kResizeDistance)) {
            return this;
        }
    }

    // Otherwise, propagate as normal.
    return LayoutWidget::widget_at(pos);
}

}  // namespace gui
