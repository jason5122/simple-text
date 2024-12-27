#pragma once

#include "gui/widget/container/horizontal_layout_widget.h"

namespace gui {

class HorizontalResizingWidget : public HorizontalLayoutWidget {
public:
    HorizontalResizingWidget(int padding_in_between = 0,
                             int left_padding = 0,
                             int right_padding = 0,
                             int top_padding = 0);

    void leftMouseDown(const app::Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;
    void leftMouseDrag(const app::Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;
    app::CursorStyle cursorStyle() const override;
    Widget* widgetAt(const app::Point& pos) override;

    constexpr std::string_view className() const override {
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
