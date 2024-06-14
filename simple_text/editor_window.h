#pragma once

#include "base/buffer.h"
#include "config/color_scheme.h"
#include "gui/window.h"

class EditorApp;

class EditorWindow : public gui::Window {
public:
    EditorWindow(EditorApp& parent, int width, int height, int wid);

    void onOpenGLActivate(int width, int height) override;
    void onDraw(int width, int height) override;
    void onResize(int width, int height) override;
    void onScroll(int dx, int dy) override;
    void onClose() override;

private:
    int wid;
    EditorApp& parent;

    base::Buffer buffer;
    config::ColorScheme color_scheme;

    void updateWindowTitle();
};
