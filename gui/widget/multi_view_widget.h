#pragma once

#include "base/numeric/literals.h"
#include "base/numeric/saturation_arithmetic.h"
#include "base/numeric/wrap_arithmetic.h"
#include "gui/widget/container_widget.h"
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
        index = base::dec_wrap(index, views.size());
    }

    void nextIndex() {
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
        views.erase(views.begin() + index);
    }

    void draw(const std::optional<Point>& mouse_pos) override {
        Widget* widget = currentWidget();
        if (widget) widget->draw(mouse_pos);
    }

    void scroll(const Point& mouse_pos, const Point& delta) override {
        Widget* widget = currentWidget();
        if (widget) widget->scroll(mouse_pos, delta);
    }

    void leftMouseDown(const Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override {
        Widget* widget = currentWidget();
        if (widget) widget->leftMouseDown(mouse_pos, modifiers, click_type);
    }

    void leftMouseDrag(const Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override {
        Widget* widget = currentWidget();
        if (widget) widget->leftMouseDrag(mouse_pos, modifiers, click_type);
    }

    void layout() override {
        for (auto& text_view : views) {
            text_view->setSize(size);
            text_view->setPosition(position);
        }
    }

    Widget* getWidgetAtPosition(const Point& pos) override {
        Widget* widget = currentWidget();
        if (widget) {
            return widget->getWidgetAtPosition(pos);
        } else {
            return nullptr;
        }
    }

    std::string_view getClassName() const override {
        return "MultiViewWidget";
    };

private:
    size_t index = 0;
    std::vector<std::shared_ptr<WidgetType>> views;
};

}
