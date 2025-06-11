#include "gui/widget/container/layout_widget.h"

namespace gui {

void LayoutWidget::set_main_widget(std::unique_ptr<Widget> widget) {
    main_widget = std::move(widget);
    layout();
}

void LayoutWidget::add_child_start(std::unique_ptr<Widget> widget) {
    children_start.emplace_back(std::move(widget));
    layout();
}

void LayoutWidget::add_child_end(std::unique_ptr<Widget> widget) {
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

void LayoutWidget::perform_scroll(const Point& mouse_pos, const Delta& delta) {
    if (main_widget) {
        if (main_widget->hit_test(mouse_pos)) {
            main_widget->perform_scroll(mouse_pos, delta);
        }
    }
    for (auto& child : children_start) {
        if (child->hit_test(mouse_pos)) {
            child->perform_scroll(mouse_pos, delta);
        }
    }
    for (auto& child : children_end) {
        if (child->hit_test(mouse_pos)) {
            child->perform_scroll(mouse_pos, delta);
        }
    }
}

void LayoutWidget::left_mouse_down(const Point& mouse_pos,
                                   ModifierKey modifiers,
                                   ClickType click_type) {
    if (main_widget) {
        main_widget->left_mouse_down(mouse_pos, modifiers, click_type);
    }
    for (auto& child : children_start) {
        child->left_mouse_down(mouse_pos, modifiers, click_type);
    }
    for (auto& child : children_end) {
        child->left_mouse_down(mouse_pos, modifiers, click_type);
    }
}

void LayoutWidget::left_mouse_drag(const Point& mouse_pos,
                                   ModifierKey modifiers,
                                   ClickType click_type) {
    if (main_widget) {
        main_widget->left_mouse_drag(mouse_pos, modifiers, click_type);
    }
    for (auto& child : children_start) {
        child->left_mouse_drag(mouse_pos, modifiers, click_type);
    }
    for (auto& child : children_end) {
        child->left_mouse_drag(mouse_pos, modifiers, click_type);
    }
}

void LayoutWidget::left_mouse_up(const Point& mouse_pos) {
    if (main_widget) {
        main_widget->left_mouse_up(mouse_pos);
    }
    for (auto& child : children_start) {
        child->left_mouse_up(mouse_pos);
    }
    for (auto& child : children_end) {
        child->left_mouse_up(mouse_pos);
    }
}

bool LayoutWidget::mouse_position_changed(const std::optional<Point>& mouse_pos) {
    bool result = false;
    if (main_widget) {
        result = main_widget->mouse_position_changed(mouse_pos) || result;
    }
    for (auto& child : children_start) {
        result = child->mouse_position_changed(mouse_pos) || result;
    }
    for (auto& child : children_end) {
        result = child->mouse_position_changed(mouse_pos) || result;
    }
    return result;
}

void LayoutWidget::set_position(const Point& position) {
    Widget::set_position(position);
    layout();
}

Widget* LayoutWidget::widget_at(const Point& pos) {
    if (main_widget) {
        if (main_widget->hit_test(pos)) {
            return main_widget->widget_at(pos);
        }
    }
    for (auto& child : children_start) {
        if (child->hit_test(pos)) {
            return child->widget_at(pos);
        }
    }
    for (auto& child : children_end) {
        if (child->hit_test(pos)) {
            return child->widget_at(pos);
        }
    }
    return nullptr;
}

}  // namespace gui
