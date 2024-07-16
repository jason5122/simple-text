#pragma once

#include "app/window.h"
#include "gui/widget/container_widget.h"
#include "gui/widget/editor_widget.h"

class EditorApp;

class EditorWindow : public app::Window {
public:
    EditorWindow(EditorApp& parent, int width, int height, int wid);

    void onOpenGLActivate(int width, int height) override;
    void onDraw(int width, int height) override;
    void onResize(int width, int height) override;
    void onScroll(int mouse_x, int mouse_y, int dx, int dy) override;
    void onLeftMouseDown(int mouse_x,
                         int mouse_y,
                         app::ModifierKey modifiers,
                         app::ClickType click_type) override;
    void onLeftMouseUp() override;
    void onLeftMouseDrag(int mouse_x, int mouse_y, app::ModifierKey modifiers) override;
    bool onKeyDown(app::Key key, app::ModifierKey modifiers) override;
    void onInsertText(std::string_view text) override;
    void onAction(app::Action action) override;
    void onAppAction(app::AppAction action) override;
    void onClose() override;

private:
    int wid;
    EditorApp& parent;
    std::shared_ptr<gui::ContainerWidget> main_widget;
    std::shared_ptr<gui::EditorWidget> editor_widget;

    // Drag selection.
    gui::Widget* drag_start_widget = nullptr;

    void updateWindowTitle();
};
