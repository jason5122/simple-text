#include "vertical_resizing_widget.h"

namespace gui {

void VerticalResizingWidget::leftMouseDown(const app::Point& mouse_pos,
                                           app::ModifierKey modifiers,
                                           app::ClickType click_type) {
    for (auto& child : children_start) {
        if (child->bottomEdgeTest(mouse_pos, kResizeDistance)) {
            dragged_widget = child.get();
            is_dragged_widget_start = true;
            return;
        }
    }
    for (auto& child : children_end) {
        if (child->topEdgeTest(mouse_pos, kResizeDistance)) {
            dragged_widget = child.get();
            is_dragged_widget_start = false;
            return;
        }
    }
}

void VerticalResizingWidget::leftMouseDrag(const app::Point& mouse_pos,
                                           app::ModifierKey modifiers,
                                           app::ClickType click_type) {
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

app::CursorStyle VerticalResizingWidget::cursorStyle() const {
    return app::CursorStyle::kResizeUpDown;
}

Widget* VerticalResizingWidget::widgetAt(const app::Point& pos) {
    // If mouse cursor is over a resizable widget edge, return this widget for resizing purposes.
    for (auto& child : children_start) {
        if (child->bottomEdgeTest(pos, kResizeDistance)) {
            return this;
        }
    }
    for (auto& child : children_end) {
        if (child->topEdgeTest(pos, kResizeDistance)) {
            return this;
        }
    }

    // Otherwise, propagate as normal.
    return LayoutWidget::widgetAt(pos);
}

}  // namespace gui
