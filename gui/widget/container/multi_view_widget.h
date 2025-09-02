#pragma once

#include "base/numeric/saturation_arithmetic.h"
#include "gui/widget/container/container_widget.h"
#include <memory>
#include <vector>

namespace gui {

template <typename WidgetType>
class MultiViewWidget : public ContainerWidget {
public:
    WidgetType* current_widget() const { return views_.empty() ? nullptr : views_[index_].get(); }

    constexpr size_t count() const { return views_.size(); }

    WidgetType* at(size_t i) { return views_[i].get(); }

    void set_index(size_t index) { index_ = index; }

    void prev_index() { index_ = (index_ + views_.size() - 1) % views_.size(); }

    void next_index() { index_ = (index_ + 1) % views_.size(); }

    void last_index() { index_ = base::sub_sat(views_.size(), size_t{1}); }

    size_t index() { return index_; }

    void add_tab(std::unique_ptr<WidgetType> widget) { views_.emplace_back(std::move(widget)); }

    void remove_tab(size_t index) {
        if (views_.empty()) return;

        views_.erase(views_.begin() + index);
        index_ = std::clamp(index, size_t{0}, base::sub_sat(views_.size(), size_t{1}));
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
        return widget ? widget->mouse_position_changed(mouse_pos) : false;
    }

    void layout() override {
        for (auto& text_view : views_) {
            text_view->set_size(size());
            text_view->set_position(position());
        }
    }

    Widget* widget_at(const Point& pos) override {
        Widget* widget = current_widget();
        return widget ? widget->widget_at(pos) : nullptr;
    }

    constexpr std::string_view class_name() const override { return "MultiViewWidget"; }

private:
    size_t index_ = 0;
    std::vector<std::unique_ptr<WidgetType>> views_;
};

}  // namespace gui
