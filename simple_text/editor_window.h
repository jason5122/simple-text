#pragma once

#include "gui/window.h"

class SimpleText;

class EditorWindow : public gui::Window {
public:
    int wid;

    NOT_COPYABLE(EditorWindow)
    NOT_MOVABLE(EditorWindow)
    EditorWindow(SimpleText& parent, int width, int height, int wid);
    ~EditorWindow() override;

    void onOpenGLActivate(int width, int height) override;
    void onDraw() override;
    void onResize(int width, int height) override;
    void onClose() override;

private:
    SimpleText& parent;
};
