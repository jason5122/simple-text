#pragma once

#include "gui/widget/container/container_widget.h"

#include <memory>
#include <vector>

namespace gui {

class LayoutWidget : public ContainerWidget {
public:
    LayoutWidget() = default;
    LayoutWidget(const Size& size) : ContainerWidget{size} {}
    virtual ~LayoutWidget() {}

    void setMainWidget(std::unique_ptr<Widget> widget);
    void addChildStart(std::unique_ptr<Widget> widget);
    void addChildEnd(std::unique_ptr<Widget> widget);

    void draw() override;
    void perform_scroll(const Point& mouse_pos, const Delta& delta) override;
    void left_mouse_down(const Point& mouse_pos,
                       ModifierKey modifiers,
                       ClickType click_type) override;
    void left_mouse_drag(const Point& mouse_pos,
                       ModifierKey modifiers,
                       ClickType click_type) override;
    void left_mouse_up(const Point& mouse_pos) override;
    bool mouse_position_changed(const std::optional<Point>& mouse_pos) override;
    void set_position(const Point& position) override;
    Widget* widget_at(const Point& pos) override;

    constexpr std::string_view class_name() const override {
        return "ContainerWidget";
    }

protected:
    std::unique_ptr<Widget> main_widget;
    std::vector<std::unique_ptr<Widget>> children_start;
    std::vector<std::unique_ptr<Widget>> children_end;
};

static_assert(std::is_abstract_v<LayoutWidget>);

}  // namespace gui
