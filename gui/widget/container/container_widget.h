#pragma once

#include "gui/widget/widget.h"

namespace gui {

class ContainerWidget : public Widget {
public:
    ContainerWidget() = default;
    ContainerWidget(const Size& size) : Widget{size} {}
    virtual ~ContainerWidget() {}

    void scroll(const Point& mouse_pos, const Delta& delta) override = 0;
    void leftMouseDown(const Point& mouse_pos,
                       ModifierKey modifiers,
                       ClickType click_type) override = 0;
    void leftMouseDrag(const Point& mouse_pos,
                       ModifierKey modifiers,
                       ClickType click_type) override = 0;
    bool mousePositionChanged(const std::optional<Point>& mouse_pos) override = 0;
    void layout() override = 0;
    Widget* widgetAt(const Point& pos) override = 0;
};

static_assert(std::is_abstract_v<ContainerWidget>);

}  // namespace gui
