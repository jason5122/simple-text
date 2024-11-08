#pragma once

#include "gui/widget/widget.h"

namespace gui {

class ContainerWidget : public Widget {
public:
    ContainerWidget() = default;
    ContainerWidget(const app::Size& size) : Widget{size} {}
    virtual ~ContainerWidget() {}

    void scroll(const app::Point& mouse_pos, const app::Delta& delta) override = 0;
    void leftMouseDown(const app::Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override = 0;
    void leftMouseDrag(const app::Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override = 0;
    bool mousePositionChanged(const std::optional<app::Point>& mouse_pos) override = 0;
    void layout() override = 0;
    Widget* getWidgetAtPosition(const app::Point& pos) override = 0;
};

static_assert(std::is_abstract<ContainerWidget>());

}  // namespace gui
