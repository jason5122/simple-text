#pragma once

#include "gui/widget.h"
#include <memory>
#include <vector>

namespace gui {

class ContainerWidget : public Widget {
public:
    ContainerWidget(const renderer::Size& size) : Widget{size} {}
    virtual ~ContainerWidget() {}

    void scroll(const renderer::Point& delta) override {
        if (main_widget) {
            main_widget->scroll(delta);
        }
        for (auto& child : children) {
            child->scroll(delta);
        }
    }
    void leftMouseDown(const renderer::Point& mouse) override {
        if (main_widget) {
            main_widget->leftMouseDown(mouse);
        }
        for (auto& child : children) {
            child->leftMouseDown(mouse);
        }
    }
    void leftMouseDrag(const renderer::Point& mouse) override {
        if (main_widget) {
            main_widget->leftMouseDrag(mouse);
        }
        for (auto& child : children) {
            child->leftMouseDrag(mouse);
        }
    }

    virtual void setMainWidget(std::unique_ptr<Widget> widget) = 0;
    virtual void addChild(std::unique_ptr<Widget> widget) = 0;

protected:
    std::unique_ptr<Widget> main_widget;
    std::vector<std::unique_ptr<Widget>> children;
};

}
