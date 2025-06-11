#pragma once

#include "base/numeric/literals.h"
#include "base/numeric/saturation_arithmetic.h"
#include "base/numeric/wrap_arithmetic.h"
#include "gui/widget/container/container_widget.h"
#include <memory>
#include <vector>

namespace gui {

template <typename WidgetType>
class MultiViewWidget : public ContainerWidget {
public:
    WidgetType* current_widget() const {
        if (!views.empty()) {
            return views[index_].get();
        } else {
            return nullptr;
        }
    }

    constexpr size_t count() const { return views.size(); }

    WidgetType* at(size_t i) { return views[i].get(); }

    void set_index(size_t index) {
        if (index < views.size()) {
            index_ = index;
        }
    }

    void prev_index() {
        if (index_ >= views.size()) return;
        index_ = base::dec_wrap(index_, views.size());
    }

    void next_index() {
        if (index_ >= views.size()) return;
        index_ = base::inc_wrap(index_, views.size());
    }

    void last_index() { index_ = base::sub_sat(views.size(), 1_Z); }

    size_t index() { return index_; }

    void addTab(std::unique_ptr<WidgetType> widget) { views.emplace_back(std::move(widget)); }

    void removeTab(size_t index) {
        if (views.empty()) return;

        views.erase(views.begin() + index);
        index_ = std::clamp(index, 0_Z, base::sub_sat(views.size(), 1_Z));
    }

    void draw() override {
        Widget* widget = current_widget();
        if (widget) widget->draw();
    }

    void perform_scroll(const Point& mouse_pos, const Delta& delta) override {
        Widget* widget = current_widget();
        if (widget) widget->perform_scroll(mouse_pos, delta);
    }

    void left_mouse_down(const Point& mouse_pos,
                         ModifierKey modifiers,
                         ClickType click_type) override {
        Widget* widget = current_widget();
        if (widget) widget->left_mouse_down(mouse_pos, modifiers, click_type);
    }

    void left_mouse_drag(const Point& mouse_pos,
                         ModifierKey modifiers,
                         ClickType click_type) override {
        Widget* widget = current_widget();
        if (widget) widget->left_mouse_drag(mouse_pos, modifiers, click_type);
    }

    void left_mouse_up(const Point& mouse_pos) override {
        Widget* widget = current_widget();
        if (widget) widget->left_mouse_up(mouse_pos);
    }

    bool mouse_position_changed(const std::optional<Point>& mouse_pos) override {
        Widget* widget = current_widget();
        if (widget) {
            return widget->mouse_position_changed(mouse_pos);
        } else {
            return false;
        }
    }

    void layout() override {
        for (auto& text_view : views) {
            text_view->set_size(size());
            text_view->set_position(position());
        }
    }

    Widget* widget_at(const Point& pos) override {
        Widget* widget = current_widget();
        if (widget) {
            return widget->widget_at(pos);
        } else {
            return nullptr;
        }
    }

    constexpr std::string_view class_name() const override { return "MultiViewWidget"; }

private:
    size_t index_ = 0;
    std::vector<std::unique_ptr<WidgetType>> views;
};

}  // namespace gui
