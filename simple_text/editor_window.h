#pragma once

#include "base/buffer.h"
#include "config/color_scheme.h"
#include "gui/window.h"
#include "renderer/types.h"

class EditorApp;

class EditorWindow : public gui::Window {
public:
    EditorWindow(EditorApp& parent, int width, int height, int wid);

    void onOpenGLActivate(int width, int height) override;
    void onDraw(int width, int height) override;
    void onResize(int width, int height) override;
    void onScroll(int dx, int dy) override;
    void onLeftMouseDown(int mouse_x, int mouse_y, gui::ModifierKey modifiers,
                         gui::ClickType click_type) override;
    void onLeftMouseDrag(int mouse_x, int mouse_y, gui::ModifierKey modifiers) override;
    void onClose() override;

private:
    int wid;
    EditorApp& parent;

    base::Buffer buffer;
    config::ColorScheme color_scheme;

    renderer::Point scroll_offset{};
    renderer::CaretInfo end_caret{};

    void updateWindowTitle();
};
