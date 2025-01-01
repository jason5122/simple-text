#pragma once

#include "app/window.h"

#include "gui/renderer/renderer.h"
#include "gui/widget/container/layout_widget.h"

class ResizableWidgetApp;

class ResizableWidgetWindow : public app::Window {
public:
    ResizableWidgetWindow(ResizableWidgetApp& parent, int width, int height, int wid);

    void onOpenGLActivate(const app::Size& size) override;
    void onDraw(const app::Size& size) override;
    void onResize(const app::Size& size) override;
    void onScroll(const app::Point& mouse_pos, const app::Delta& delta) override;

    void onLeftMouseDown(const app::Point& mouse_pos,
                         app::ModifierKey modifiers,
                         app::ClickType click_type) override;
    void onLeftMouseUp(const app::Point& mouse_pos) override;
    void onLeftMouseDrag(const app::Point& mouse_pos,
                         app::ModifierKey modifiers,
                         app::ClickType click_type) override;
    void onMouseMove(const app::Point& mouse_pos) override;

private:
    static constexpr gui::Rgba kGold = {255, 215, 0, 255};
    static constexpr gui::Rgba kPurple = {147, 112, 219, 255};
    static constexpr gui::Rgba kSandyBrown = {244, 164, 96, 255};
    static constexpr gui::Rgba kLightBlue = {176, 196, 222, 255};
    static constexpr gui::Rgba kSeaGreen = {60, 179, 113, 255};

    ResizableWidgetApp& parent [[maybe_unused]];

    std::unique_ptr<gui::LayoutWidget> main_widget;

    // The widget that the drag operation was performed on. If there currently isn't a drag
    // operation, this is null.
    gui::Widget* dragged_widget = nullptr;

    void updateCursorStyle(const std::optional<app::Point>& mouse_pos);
};
