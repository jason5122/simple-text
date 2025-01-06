#include "vertical_resizing_widget.h"

#include <fmt/base.h>

namespace gui {

void VerticalResizingWidget::leftMouseDown(const Point& mouse_pos,
                                           ModifierKey modifiers,
                                           ClickType click_type) {
    for (auto& child : children_start) {
        if (child->bottomEdgeTest(mouse_pos, kResizeDistance)) {
            if (click_type == ClickType::kDoubleClick) {
                child->setHeight(child->getMinimumHeight());
            } else {
                dragged_widget = child.get();
                is_dragged_widget_start = true;
                return;
            }
        }
    }
    for (auto& child : children_end) {
        if (child->topEdgeTest(mouse_pos, kResizeDistance)) {
            if (click_type == ClickType::kDoubleClick) {
                child->setHeight(child->getMinimumHeight());
            } else {
                dragged_widget = child.get();
                is_dragged_widget_start = false;
                return;
            }
        }
    }
}

void VerticalResizingWidget::leftMouseDrag(const Point& mouse_pos,
                                           ModifierKey modifiers,
                                           ClickType click_type) {
    // TODO: If a double click occurred, prevent the drag from starting in the first place.
    if (click_type == ClickType::kDoubleClick) {
        return;
    }

    if (dragged_widget) {
        auto widget_size = dragged_widget->getSize();
        auto widget_pos = dragged_widget->getPosition();

        int new_height;
        if (is_dragged_widget_start) {
            new_height = mouse_pos.y - widget_pos.y;
        } else {
            new_height = (widget_pos.y + widget_size.height) - mouse_pos.y;
        }
        dragged_widget->setHeight(new_height);
    }
}

CursorStyle VerticalResizingWidget::cursorStyle() const {
    return CursorStyle::kResizeUpDown;
}

Widget* VerticalResizingWidget::widgetAt(const Point& pos) {
    // If mouse cursor is over a resizable widget edge, return this widget for resizing purposes.
    for (auto& child : children_start) {
        if (child->isResizable() && child->bottomEdgeTest(pos, kResizeDistance)) {
            return this;
        }
    }
    for (auto& child : children_end) {
        if (child->isResizable() && child->topEdgeTest(pos, kResizeDistance)) {
            return this;
        }
    }

    // Otherwise, propagate as normal.
    return LayoutWidget::widgetAt(pos);
}

}  // namespace gui
