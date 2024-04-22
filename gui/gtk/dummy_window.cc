#include "dummy_window.h"
#include <glad/glad.h>

#include <iostream>

static void realize(GtkWidget* self, gpointer user_data);
static gboolean draw(GtkWidget* widget, cairo_t* cr, gpointer user_data);

DummyWindow::DummyWindow() {
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window, "realize", G_CALLBACK(realize), this);
    g_signal_connect(window, "draw", G_CALLBACK(draw), this);
}

void DummyWindow::show() {
    gtk_widget_show_all(window);
}

static void realize(GtkWidget* self, gpointer user_data) {
    DummyWindow* dummy_window = static_cast<DummyWindow*>(user_data);

    std::cerr << "realize\n";

    EGLConfig egl_config;
    EGLint n_config;
    EGLint attributes[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, EGL_NONE};

    dummy_window->egl_display = (EGLDisplay*)eglGetDisplay(
        (EGLNativeDisplayType)gdk_wayland_display_get_wl_display(gtk_widget_get_display(self)));
    eglInitialize(dummy_window->egl_display, nullptr, nullptr);
    eglChooseConfig(dummy_window->egl_display, attributes, &egl_config, 1, &n_config);
    eglBindAPI(EGL_OPENGL_API);
    dummy_window->egl_surface = (EGLSurface*)eglCreateWindowSurface(
        dummy_window->egl_display, egl_config,
        (EGLNativeWindowType)gdk_wayland_window_get_wl_surface(gtk_widget_get_window(self)), NULL);
    dummy_window->egl_context =
        (EGLContext*)eglCreateContext(dummy_window->egl_display, egl_config, EGL_NO_CONTEXT, NULL);

    eglMakeCurrent(dummy_window->egl_display, dummy_window->egl_surface, dummy_window->egl_surface,
                   dummy_window->egl_context);

    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD.\n";
    }

    glClearColor(0, 1, 0, 1);
}

static gboolean draw(GtkWidget* widget, cairo_t* cr, gpointer user_data) {
    DummyWindow* dummy_window = static_cast<DummyWindow*>(user_data);

    // eglMakeCurrent(dummy_window->egl_display, dummy_window->egl_surface,
    // dummy_window->egl_surface,
    //                dummy_window->egl_context);

    // glClear(GL_COLOR_BUFFER_BIT);
    // std::cerr << "draw " << dummy_window->temp << '\n';

    // eglSwapBuffers(dummy_window->egl_display, dummy_window->egl_surface);

    guint width, height;
    GdkRGBA color;
    GtkStyleContext* context;

    context = gtk_widget_get_style_context(widget);

    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);

    gtk_render_background(context, cr, 0, 0, width, height);

    cairo_arc(cr, width / 2.0, height / 2.0, MIN(width, height) / 2.0, 0, 2 * G_PI);

    gtk_style_context_get_color(context, gtk_style_context_get_state(context), &color);
    gdk_cairo_set_source_rgba(cr, &color);

    cairo_fill(cr);

    return false;
}
