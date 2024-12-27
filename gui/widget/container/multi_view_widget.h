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
    WidgetType* currentWidget() const {
        if (!views.empty()) {
            return views[index].get();
        } else {
            return nullptr;
        }
    }

    void setIndex(size_t index) {
        if (index < views.size()) {
            this->index = index;
        }
    }

    void prevIndex() {
        if (index >= views.size()) return;
        index = base::dec_wrap(index, views.size());
    }

    void nextIndex() {
        if (index >= views.size()) return;
        index = base::inc_wrap(index, views.size());
    }

    void lastIndex() {
        index = base::sub_sat(views.size(), 1_Z);
    }

    size_t getCurrentIndex() {
        return index;
    }

    void addTab(std::shared_ptr<WidgetType> widget) {
        views.emplace_back(std::move(widget));
    }

    void removeTab(size_t index) {
        if (views.empty()) return;

        views.erase(views.begin() + index);
        this->index = std::clamp(index, 0_Z, base::sub_sat(views.size(), 1_Z));
    }

    void draw() override {
        Widget* widget = currentWidget();
        if (widget) widget->draw();
    }

    void scroll(const app::Point& mouse_pos, const app::Delta& delta) override {
        Widget* widget = currentWidget();
        if (widget) widget->scroll(mouse_pos, delta);
    }

    void leftMouseDown(const app::Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override {
        Widget* widget = currentWidget();
        if (widget) widget->leftMouseDown(mouse_pos, modifiers, click_type);
    }

    void leftMouseDrag(const app::Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override {
        Widget* widget = currentWidget();
        if (widget) widget->leftMouseDrag(mouse_pos, modifiers, click_type);
    }

    bool mousePositionChanged(const std::optional<app::Point>& mouse_pos) override {
        Widget* widget = currentWidget();
        if (widget) {
            return widget->mousePositionChanged(mouse_pos);
        } else {
            return false;
        }
    }

    void layout() override {
        for (auto& text_view : views) {
            text_view->setSize(size);
            text_view->setPosition(position);
        }
    }

    Widget* widgetAt(const app::Point& pos) override {
        Widget* widget = currentWidget();
        if (widget) {
            return widget->widgetAt(pos);
        } else {
            return nullptr;
        }
    }

    constexpr std::string_view className() const override {
        return "MultiViewWidget";
    }

private:
    size_t index = 0;
    std::vector<std::shared_ptr<WidgetType>> views;
};

}  // namespace gui
