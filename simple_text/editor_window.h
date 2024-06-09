#pragma once

#include "config/color_scheme.h"
#include "gui/window.h"

class SimpleText;

class EditorWindow : public gui::Window {
public:
    int wid;

    EditorWindow(SimpleText& parent, int width, int height, int wid);
    ~EditorWindow() override;

    void onOpenGLActivate(int width, int height) override;
    void onDraw() override;
    void onResize(int width, int height) override;
    void onClose() override;

private:
    SimpleText& parent;

    int tab_index = 0;  // Use int instead of size_t to prevent wrap around when less than 0.
    // std::vector<std::unique_ptr<EditorTab>> tabs;

    config::ColorScheme color_scheme;

    void updateWindowTitle();
};
