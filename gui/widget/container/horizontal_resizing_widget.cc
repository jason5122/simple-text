#include "horizontal_resizing_widget.h"

namespace gui {

HorizontalResizingWidget::HorizontalResizingWidget(int padding_in_between,
                                                   int left_padding,
                                                   int right_padding,
                                                   int top_padding)
    : HorizontalLayoutWidget(padding_in_between, left_padding, right_padding, top_padding) {}

void HorizontalResizingWidget::leftMouseDown(const app::Point& mouse_pos,
                                             app::ModifierKey modifiers,
                                             app::ClickType click_type) {
    for (auto& child : children_start) {
        if (child->rightEdgeTest(mouse_pos, kResizeDistance)) {
            dragged_widget = child.get();
            is_dragged_widget_start = true;
            return;
        }
    }
    for (auto& child : children_end) {
        if (child->leftEdgeTest(mouse_pos, kResizeDistance)) {
            dragged_widget = child.get();
            is_dragged_widget_start = false;
            return;
        }
    }
}

void HorizontalResizingWidget::leftMouseDrag(const app::Point& mouse_pos,
                                             app::ModifierKey modifiers,
                                             app::ClickType click_type) {
    if (dragged_widget) {
        auto widget_size = dragged_widget->getSize();
        auto widget_pos = dragged_widget->getPosition();

        int new_width;
        if (is_dragged_widget_start) {
            new_width = mouse_pos.x - widget_pos.x;
        } else {
            new_width = (widget_pos.x + widget_size.width) - mouse_pos.x;
        }
        dragged_widget->setWidth(new_width);
    }
}

app::CursorStyle HorizontalResizingWidget::cursorStyle() const {
    return app::CursorStyle::kResizeLeftRight;
}

Widget* HorizontalResizingWidget::widgetAt(const app::Point& pos) {
    // If mouse cursor is over a resizable widget edge, return this widget for resizing purposes.
    for (auto& child : children_start) {
        if (child->isResizable() && child->rightEdgeTest(pos, kResizeDistance)) {
            return this;
        }
    }
    for (auto& child : children_end) {
        if (child->isResizable() && child->leftEdgeTest(pos, kResizeDistance)) {
            return this;
        }
    }

    // Otherwise, propagate as normal.
    return LayoutWidget::widgetAt(pos);
}

}  // namespace gui
