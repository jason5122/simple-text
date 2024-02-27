#pragma once

#include <gtk/gtk.h>

class EditorWindow {
public:
    EditorWindow();
    ~EditorWindow();
    int run();

private:
    GtkApplication* app;
};
