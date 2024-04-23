#pragma once

#include "gui/app.h"
#include <gtk/gtk.h>

class MainWindow {
public:
    App::Window* app_window;

    MainWindow(GtkApplication* gtk_app, App::Window* app_window, GdkGLContext* gl_context);
    void show();
    void close();
    void redraw();
    int width();
    int height();
    int scaleFactor();

    // private:
    GtkWidget* window;
    GtkWidget* gl_area;
    GdkGLContext* gl_context;
};
