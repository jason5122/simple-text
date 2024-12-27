#include "layout_widget.h"

namespace gui {

void LayoutWidget::setMainWidget(std::unique_ptr<Widget> widget) {
    main_widget = std::move(widget);
    layout();
}

void LayoutWidget::addChildStart(std::unique_ptr<Widget> widget) {
    children_start.emplace_back(std::move(widget));
    layout();
}

void LayoutWidget::addChildEnd(std::unique_ptr<Widget> widget) {
    children_end.emplace_back(std::move(widget));
    layout();
}

void LayoutWidget::draw() {
    if (main_widget) {
        main_widget->draw();
    }
    for (auto& child : children_start) {
        child->draw();
    }
    for (auto& child : children_end) {
        child->draw();
    }
}

void LayoutWidget::scroll(const app::Point& mouse_pos, const app::Delta& delta) {
    if (main_widget) {
        if (main_widget->hitTest(mouse_pos)) {
            main_widget->scroll(mouse_pos, delta);
        }
    }
    for (auto& child : children_start) {
        if (child->hitTest(mouse_pos)) {
            child->scroll(mouse_pos, delta);
        }
    }
    for (auto& child : children_end) {
        if (child->hitTest(mouse_pos)) {
            child->scroll(mouse_pos, delta);
        }
    }
}

void LayoutWidget::leftMouseDown(const app::Point& mouse_pos,
                                 app::ModifierKey modifiers,
                                 app::ClickType click_type) {
    if (main_widget) {
        main_widget->leftMouseDown(mouse_pos, modifiers, click_type);
    }
    for (auto& child : children_start) {
        child->leftMouseDown(mouse_pos, modifiers, click_type);
    }
    for (auto& child : children_end) {
        child->leftMouseDown(mouse_pos, modifiers, click_type);
    }
}

void LayoutWidget::leftMouseDrag(const app::Point& mouse_pos,
                                 app::ModifierKey modifiers,
                                 app::ClickType click_type) {
    if (main_widget) {
        main_widget->leftMouseDrag(mouse_pos, modifiers, click_type);
    }
    for (auto& child : children_start) {
        child->leftMouseDrag(mouse_pos, modifiers, click_type);
    }
    for (auto& child : children_end) {
        child->leftMouseDrag(mouse_pos, modifiers, click_type);
    }
}

bool LayoutWidget::mousePositionChanged(const std::optional<app::Point>& mouse_pos) {
    bool result = false;
    if (main_widget) {
        result = main_widget->mousePositionChanged(mouse_pos) || result;
    }
    for (auto& child : children_start) {
        result = child->mousePositionChanged(mouse_pos) || result;
    }
    for (auto& child : children_end) {
        result = child->mousePositionChanged(mouse_pos) || result;
    }
    return result;
}

void LayoutWidget::setPosition(const app::Point& position) {
    this->position = position;
    layout();
}

Widget* LayoutWidget::widgetAt(const app::Point& pos) {
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
