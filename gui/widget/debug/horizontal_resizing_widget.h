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
    app::CursorStyle cursorStyle() const override;
    Widget* widgetAt(const app::Point& pos) override;

    std::string_view className() const override {
        return "HorizontalResizingWidget";
    }

private:
    // Max distance the mouse cursor can be from a resizable widget edge to initiate a resize.
    static constexpr int kResizeDistance = 3 * 2;

    // The widget that the drag operation was performed on. If there currently isn't a drag
    // operation, this is null.
    gui::Widget* dragged_widget = nullptr;
    bool is_dragged_widget_start;
};

}  // namespace gui
