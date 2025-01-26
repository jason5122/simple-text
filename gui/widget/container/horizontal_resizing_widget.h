#pragma once

#include "gui/widget/container/horizontal_layout_widget.h"

namespace gui {

class HorizontalResizingWidget : public HorizontalLayoutWidget {
public:
    HorizontalResizingWidget(int padding_in_between = 0,
                             int left_padding = 0,
                             int right_padding = 0,
                             int top_padding = 0);

    void left_mouse_down(const Point& mouse_pos,
                       ModifierKey modifiers,
                       ClickType click_type) override;
    void left_mouse_drag(const Point& mouse_pos,
                       ModifierKey modifiers,
                       ClickType click_type) override;
    CursorStyle cursor_style() const override;
    Widget* widget_at(const Point& pos) override;

    constexpr std::string_view class_name() const override {
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
