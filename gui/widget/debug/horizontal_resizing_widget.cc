#include "horizontal_resizing_widget.h"

namespace gui {

HorizontalResizingWidget::HorizontalResizingWidget(const app::Size& size) : LayoutWidget{size} {}

void HorizontalResizingWidget::layout() {
    int left_offset = position.x;
    int right_offset = position.x + size.width;

    for (auto& child : children_start) {
        child->setPosition({left_offset, position.y});
        if (child->isAutoresizing()) {
            child->setHeight(size.height);
        }

        // Recursively layout children.
        child->layout();

        left_offset += child->getSize().width;
    }

    for (auto& child : children_end) {
        right_offset -= child->getSize().width;

        child->setPosition({right_offset, position.y});
        if (child->isAutoresizing()) {
            child->setHeight(size.height);
        }

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

void HorizontalResizingWidget::leftMouseDown(const app::Point& mouse_pos,
                                             app::ModifierKey modifiers,
                                             app::ClickType click_type) {
    int left_offset = position.x;
    int right_offset = position.x + size.width;
    for (auto& child : children_start) {
        left_offset += child->getSize().width;

        if (std::abs(mouse_pos.x - left_offset) <= kResizeDistance) {
            dragged_widget = child.get();
            is_dragged_widget_start = true;
            return;
        }
    }
    for (auto& child : children_end) {
        right_offset -= child->getSize().width;

        if (std::abs(mouse_pos.x - right_offset) <= kResizeDistance) {
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

        // TODO: Consider refactoring this. We probably don't need children at both start and end.
        int new_width;
        if (is_dragged_widget_start) {
            new_width = mouse_pos.x - widget_pos.x;
        } else {
            new_width = (widget_pos.x + widget_size.width) - mouse_pos.x;
        }
        new_width = std::max(new_width, 50 * 2);

        dragged_widget->setWidth(new_width);
    }
}

app::CursorStyle HorizontalResizingWidget::cursorStyle() const {
    return app::CursorStyle::kResizeLeftRight;
}

Widget* HorizontalResizingWidget::widgetAt(const app::Point& pos) {
    // If mouse cursor is over a resizable widget edge, return this widget for resizing purposes.
    int left_offset = position.x;
    int right_offset = position.x + size.width;
    for (auto& child : children_start) {
        left_offset += child->getSize().width;
        if (std::abs(pos.x - left_offset) <= kResizeDistance) {
            return this;
        }
    }
    for (auto& child : children_end) {
        right_offset -= child->getSize().width;
        if (std::abs(pos.x - right_offset) <= kResizeDistance) {
            return this;
        }
    }

    // Otherwise, propagate as normal.
    if (main_widget) {
        if (main_widget->hitTest(pos)) {
            return main_widget->widgetAt(pos);
        }
    }
    for (auto& child : children_start) {
        if (child->hitTest(pos)) {
            return child->widgetAt(pos);
        }
    }
    for (auto& child : children_end) {
        if (child->hitTest(pos)) {
            return child->widgetAt(pos);
        }
    }
    return nullptr;
}

}  // namespace gui
