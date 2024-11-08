#pragma once

#include "app/window.h"
#include "gui/widget/container/layout_widget.h"
#include "gui/widget/editor_widget.h"

class EditorApp;

class EditorWindow : public app::Window {
public:
    EditorWindow(EditorApp& parent, int width, int height, int wid);

    void onOpenGLActivate(int width, int height) override;
    void onDraw(int width, int height) override;
    void onResize(int width, int height) override;
    void onScroll(const app::Point& mouse_pos, const app::Delta& delta) override;
    void onLeftMouseDown(const app::Point& mouse_pos,
                         app::ModifierKey modifiers,
                         app::ClickType click_type) override;
    void onLeftMouseUp() override;
    void onLeftMouseDrag(const app::Point& mouse_pos,
                         app::ModifierKey modifiers,
                         app::ClickType click_type) override;
    void onRightMouseDown(const app::Point& mouse_pos,
                          app::ModifierKey modifiers,
                          app::ClickType click_type) override;
    void onMouseMove() override;
    void onMouseExit() override;
    bool onKeyDown(app::Key key, app::ModifierKey modifiers) override;
    void onInsertText(std::string_view text) override;
    void onAction(app::Action action, bool extend) override;
    void onAppAction(app::AppAction action) override;
    void onClose() override;

private:
    static constexpr int kSideBarWidth = 250 * 2;
    static constexpr int kStatusBarHeight = 22 * 2;

    int wid;
    EditorApp& parent;
    std::shared_ptr<gui::LayoutWidget> main_widget;
    std::shared_ptr<gui::EditorWidget> editor_widget;

    // Drag selection.
    gui::Widget* drag_start_widget = nullptr;

    void updateCursorStyle();
};
