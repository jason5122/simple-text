#pragma once

#include "gui/platform/window_widget.h"
#include "gui/widget/container/layout_widget.h"
#include "gui/widget/editor_widget.h"
#include "gui/widget/side_bar_widget.h"
#include "gui/widget/status_bar_widget.h"

namespace gui {

class EditorApp;

class EditorWindow : public WindowWidget {
public:
    EditorWindow(EditorApp& parent, int width, int height, int wid);

    // Widget overrides.
    void draw() override;
    void layout() override;
    void perform_scroll(const Point& mouse_pos, const Delta& delta) override;
    void left_mouse_down(const Point& mouse_pos,
                       ModifierKey modifiers,
                       ClickType click_type) override;
    void left_mouse_drag(const Point& mouse_pos,
                       ModifierKey modifiers,
                       ClickType click_type) override;
    void left_mouse_up(const Point& mouse_pos) override;
    void right_mouse_down(const Point& mouse_pos,
                        ModifierKey modifiers,
                        ClickType click_type) override;
    bool mouse_position_changed(const std::optional<Point>& mouse_pos) override;

    constexpr std::string_view class_name() const final override {
        return "EditorWindow";
    }

    // WindowWidget overrides.
    void onOpenGLActivate() override;
    void onFrame(int64_t ms) override;
    void onScrollDecelerate(const Point& mouse_pos, const Delta& delta) override;
    bool onKeyDown(Key key, ModifierKey modifiers) override;
    void onInsertText(std::string_view text) override;
    void onAction(Action action, bool extend) override;
    void onAppAction(AppAction action) override;
    void onClose() override;

private:
    static constexpr int kMinStatusBarHeight = 22 * 2;
    static constexpr int kSideBarWidth = 250 * 2;

    // TODO: Clean this up.
    static constexpr int kRatePerSec = 5;
    static constexpr int kDecelFriction = 4;
    int requested_frames = 0;
    // bool is_side_bar_animating = false;
    // bool is_side_bar_open = true;
    // int target_width = 0;
    int vel_x = 0;
    int vel_y = 0;
    // int64_t last_ms = 0;
    // int64_t ms_err = 0;
    // TODO: Remove this.
    Point last_mouse_pos;

    int wid;
    EditorApp& parent;
    std::unique_ptr<gui::LayoutWidget> main_widget;

    // These cache unique_ptrs. These are guaranteed to be non-null since they are owned by
    // `main_widget`.
    gui::EditorWidget* editor_widget;
    gui::StatusBarWidget* status_bar;
    gui::SideBarWidget* side_bar;

    // The widget that the drag operation was performed on. If there currently isn't a drag
    // operation, this is null.
    gui::Widget* dragged_widget = nullptr;
    gui::Widget* focused_widget = nullptr;

    void updateCursorStyle(const std::optional<Point>& mouse_pos);
};

}  // namespace gui
