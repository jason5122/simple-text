#pragma once

#include <EGL/egl.h>
#include <gdk/gdkwayland.h>
#include <gtk/gtk.h>

class DummyWindow {
public:
    EGLDisplay* egl_display;
    EGLSurface* egl_surface;
    EGLContext* egl_context;
    int temp = 42;

    DummyWindow();
    void show();

private:
    GtkWidget* window;
};
