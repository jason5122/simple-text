#pragma once

#include "gui/widget/widget.h"
#include <memory>
#include <vector>

namespace gui {

class ContainerWidget : public Widget {
public:
    ContainerWidget() = default;
    ContainerWidget(const renderer::Size& size) : Widget{size} {}
    virtual ~ContainerWidget() {}

    void setMainWidget(std::unique_ptr<Widget> widget);
    void addChildStart(std::unique_ptr<Widget> widget);
    void addChildEnd(std::unique_ptr<Widget> widget);
    Widget* getWidgetAtPosition(const renderer::Point& position);

    void draw() override;
    void scroll(const renderer::Point& mouse_pos, const renderer::Point& delta) override;
    void leftMouseDown(const renderer::Point& mouse_pos) override;
    void leftMouseDrag(const renderer::Point& mouse_pos) override;

    void setPosition(const renderer::Point& position) override;

protected:
    std::unique_ptr<Widget> main_widget;
    std::vector<std::unique_ptr<Widget>> children_start;
    std::vector<std::unique_ptr<Widget>> children_end;
};

}
