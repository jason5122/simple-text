#pragma once

#include "base/buffer.h"
#include "ui/renderer/rect_renderer.h"
#include <gtk/gtk.h>

class EditorWindow {
public:
    RectRenderer rect_renderer;

    EditorWindow();
    ~EditorWindow();
    int run();

private:
    GtkApplication* app;
};
