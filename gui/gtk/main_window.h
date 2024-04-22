#pragma once

#include "gui/app.h"
#include <gtk/gtk.h>

class MainWindow {
public:
    App::Window* app_window;

    MainWindow(GtkApplication* gtk_app, App::Window* app_window);
    void show();
    void close();
    void redraw();

private:
    GtkWidget* window;
    GtkWidget* gl_area;
};
