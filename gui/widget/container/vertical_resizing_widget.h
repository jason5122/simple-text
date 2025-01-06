#pragma once

#include "gui/widget/container/vertical_layout_widget.h"

namespace gui {

class VerticalResizingWidget : public VerticalLayoutWidget {
public:
    void leftMouseDown(const Point& mouse_pos,
                       ModifierKey modifiers,
                       ClickType click_type) override;
    void leftMouseDrag(const Point& mouse_pos,
                       ModifierKey modifiers,
                       ClickType click_type) override;
    CursorStyle cursorStyle() const override;
    Widget* widgetAt(const Point& pos) override;

    constexpr std::string_view className() const override {
        return "VerticalResizingWidget";
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
