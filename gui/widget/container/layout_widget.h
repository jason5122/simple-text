#pragma once

#include "gui/widget/container/container_widget.h"

#include <memory>
#include <vector>

namespace gui {

class LayoutWidget : public ContainerWidget {
public:
    LayoutWidget() = default;
    LayoutWidget(const app::Size& size) : ContainerWidget{size} {}
    virtual ~LayoutWidget() {}

    void setMainWidget(std::unique_ptr<Widget> widget);
    void addChildStart(std::unique_ptr<Widget> widget);
    void addChildEnd(std::unique_ptr<Widget> widget);

    void draw() override;
    void scroll(const app::Point& mouse_pos, const app::Delta& delta) override;
    void leftMouseDown(const app::Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;
    void leftMouseDrag(const app::Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;
    bool mousePositionChanged(const std::optional<app::Point>& mouse_pos) override;
    void setPosition(const app::Point& position) override;
    Widget* widgetAt(const app::Point& pos) override;

    constexpr std::string_view className() const override {
        return "ContainerWidget";
    }

protected:
    std::unique_ptr<Widget> main_widget;
    std::vector<std::unique_ptr<Widget>> children_start;
    std::vector<std::unique_ptr<Widget>> children_end;
};

static_assert(std::is_abstract_v<LayoutWidget>);

}  // namespace gui
