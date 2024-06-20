#pragma once

#include "app/window.h"
#include "gui/horizontal_layout_widget.h"

class EditorApp;

class EditorWindow : public app::Window {
public:
    EditorWindow(EditorApp& parent, int width, int height, int wid);

    void onOpenGLActivate(int width, int height) override;
    void onDraw(int width, int height) override;
    void onResize(int width, int height) override;
    void onScroll(int dx, int dy) override;
    void onLeftMouseDown(int mouse_x,
                         int mouse_y,
                         app::ModifierKey modifiers,
                         app::ClickType click_type) override;
    void onLeftMouseDrag(int mouse_x, int mouse_y, app::ModifierKey modifiers) override;
    void onClose() override;

private:
    int wid;
    EditorApp& parent;
    gui::HorizontalLayoutWidget main_widget;

    void updateWindowTitle();
};
