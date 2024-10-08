#include "layout_widget.h"

namespace gui {

void LayoutWidget::setMainWidget(std::shared_ptr<Widget> widget) {
    main_widget = std::move(widget);
    layout();
}

void LayoutWidget::addChildStart(std::shared_ptr<Widget> widget) {
    children_start.push_back(std::move(widget));
    layout();
}

void LayoutWidget::addChildEnd(std::shared_ptr<Widget> widget) {
    children_end.push_back(std::move(widget));
    layout();
}

void LayoutWidget::draw(const std::optional<Point>& mouse_pos) {
    if (main_widget) {
        main_widget->draw(mouse_pos);
    }
    for (auto& child : children_start) {
        child->draw(mouse_pos);
    }
    for (auto& child : children_end) {
        child->draw(mouse_pos);
    }
}

void LayoutWidget::scroll(const Point& mouse_pos, const Point& delta) {
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

void LayoutWidget::leftMouseDown(const Point& mouse_pos,
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

void LayoutWidget::leftMouseDrag(const Point& mouse_pos,
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

void LayoutWidget::mousePositionChanged(const std::optional<Point>& mouse_pos) {
    if (main_widget) {
        main_widget->mousePositionChanged(mouse_pos);
    }
    for (auto& child : children_start) {
        child->mousePositionChanged(mouse_pos);
    }
    for (auto& child : children_end) {
        child->mousePositionChanged(mouse_pos);
    }
}

void LayoutWidget::setPosition(const Point& position) {
    this->position = position;
    layout();
}

Widget* LayoutWidget::getWidgetAtPosition(const Point& pos) {
    if (main_widget) {
        if (main_widget->hitTest(pos)) {
            return main_widget->getWidgetAtPosition(pos);
        }
    }
    for (auto& child : children_start) {
        if (child->hitTest(pos)) {
            return child->getWidgetAtPosition(pos);
        }
    }
    for (auto& child : children_end) {
        if (child->hitTest(pos)) {
            return child->getWidgetAtPosition(pos);
        }
    }
    return nullptr;
}

}
