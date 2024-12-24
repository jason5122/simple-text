#pragma once

#include "gui/widget/container/layout_widget.h"

namespace gui {

class HorizontalResizingWidget : public LayoutWidget {
public:
    HorizontalResizingWidget() = default;
    HorizontalResizingWidget(const app::Size& size);

    void layout() override;
    void leftMouseDown(const app::Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;
    void leftMouseDrag(const app::Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;
    Widget* widgetAt(const app::Point& pos) override;

    std::string_view className() const override {
        return "HorizontalResizingWidget";
    }

private:
    // The widget that the drag operation was performed on. If there currently isn't a drag
    // operation, this is null.
    gui::Widget* dragged_widget = nullptr;
};

}  // namespace gui
