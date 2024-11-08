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

    void setMainWidget(std::shared_ptr<Widget> widget);
    void addChildStart(std::shared_ptr<Widget> widget);
    void addChildEnd(std::shared_ptr<Widget> widget);

    void draw(const std::optional<app::Point>& mouse_pos) override;
    void scroll(const app::Point& mouse_pos, const app::Delta& delta) override;
    void leftMouseDown(const app::Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;
    void leftMouseDrag(const app::Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;
    bool mousePositionChanged(const std::optional<app::Point>& mouse_pos) override;
    void setPosition(const app::Point& position) override;
    Widget* getWidgetAtPosition(const app::Point& pos) override;

    std::string_view getClassName() const override {
        return "ContainerWidget";
    };

protected:
    std::shared_ptr<Widget> main_widget;
    std::vector<std::shared_ptr<Widget>> children_start;
    std::vector<std::shared_ptr<Widget>> children_end;
};

static_assert(std::is_abstract<LayoutWidget>());

}  // namespace gui
