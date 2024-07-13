#pragma once

#include "gui/widget/widget.h"
#include <memory>
#include <vector>

namespace gui {

class ContainerWidget : public Widget {
public:
    ContainerWidget() = default;
    ContainerWidget(const Size& size) : Widget{size} {}
    virtual ~ContainerWidget() {}

    void setMainWidget(std::shared_ptr<Widget> widget);
    void addChildStart(std::shared_ptr<Widget> widget);
    void addChildEnd(std::shared_ptr<Widget> widget);
    Widget* getWidgetAtPosition(const Point& position);

    void draw() override;
    void scroll(const Point& mouse_pos, const Point& delta) override;
    void leftMouseDown(const Point& mouse_pos) override;
    void leftMouseDrag(const Point& mouse_pos) override;

    void setPosition(const Point& position) override;

protected:
    std::shared_ptr<Widget> main_widget;
    std::vector<std::shared_ptr<Widget>> children_start;
    std::vector<std::shared_ptr<Widget>> children_end;
};

}
