#include "horizontal_resizing_widget.h"

#include <fmt/core.h>

namespace gui {

HorizontalResizingWidget::HorizontalResizingWidget(const app::Size& size) : LayoutWidget{size} {}

void HorizontalResizingWidget::layout() {
    int left_offset = position.x;
    int right_offset = position.x + size.width;

    for (auto& child : children_start) {
        child->setPosition({left_offset, position.y});
        child->setHeight(size.height);

        // Recursively layout children.
        child->layout();

        left_offset += child->getSize().width;
    }

    for (auto& child : children_end) {
        right_offset -= child->getSize().width;

        child->setPosition({right_offset, position.y});
        child->setHeight(size.height);

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
    fmt::println("mouse_pos.x = {}", mouse_pos.x);

    int x = position.x;
    for (auto& child : children_start) {
        x += child->getSize().width;

        if (abs(mouse_pos.x - x) < 40) {
            fmt::println("x = {}", x);
            dragged_widget = child.get();
        }
    }
}

void HorizontalResizingWidget::leftMouseDrag(const app::Point& mouse_pos,
                                             app::ModifierKey modifiers,
                                             app::ClickType click_type) {
    if (dragged_widget) {
        auto widget_pos = dragged_widget->getPosition();
        int new_width = std::max(mouse_pos.x - widget_pos.x, 50 * 2);
        dragged_widget->setWidth(new_width);
    }
}

Widget* HorizontalResizingWidget::widgetAt(const app::Point& pos) {
    // If mouse cursor is over a resizable widget edge, return this widget for resizing purposes.
    int x = position.x;
    for (auto& child : children_start) {
        x += child->getSize().width;

        if (abs(pos.x - x) < 40) {
            return this;
        }
    }
    // TODO: Also do this for `children_end`.

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
