#pragma once

#include "app/window.h"
#include "gui/widget/container/layout_widget.h"
#include "gui/widget/editor_widget.h"
#include "gui/widget/side_bar_widget.h"
#include "gui/widget/status_bar_widget.h"

class EditorApp;

class EditorWindow : public app::Window {
public:
    EditorWindow(EditorApp& parent, int width, int height, int wid);

    void onOpenGLActivate(const app::Size& size) override;
    void onDraw(const app::Size& size) override;
    void onFrame(int64_t ms) override;
    void onResize(const app::Size& size) override;
    void onScroll(const app::Point& mouse_pos, const app::Delta& delta) override;
    void onScrollDecelerate(const app::Point& mouse_pos, const app::Delta& delta) override;
    void onLeftMouseDown(const app::Point& mouse_pos,
                         app::ModifierKey modifiers,
                         app::ClickType click_type) override;
    void onLeftMouseUp(const app::Point& mouse_pos) override;
    void onLeftMouseDrag(const app::Point& mouse_pos,
                         app::ModifierKey modifiers,
                         app::ClickType click_type) override;
    void onRightMouseDown(const app::Point& mouse_pos,
                          app::ModifierKey modifiers,
                          app::ClickType click_type) override;
    void onMouseMove(const app::Point& mouse_pos) override;
    void onMouseExit() override;
    bool onKeyDown(app::Key key, app::ModifierKey modifiers) override;
    void onInsertText(std::string_view text) override;
    void onAction(app::Action action, bool extend) override;
    void onAppAction(app::AppAction action) override;
    void onClose() override;

private:
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
    app::Point last_mouse_pos;

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

    void updateCursorStyle(const std::optional<app::Point>& mouse_pos);
};
