#include "dummy_window.h"
#include <EGL/egl.h>
#include <glad/glad.h>
#include <iostream>

static void realize_window(GtkWidget* self, gpointer user_data);
static void realize(GtkWidget* self, gpointer user_data);
static gboolean render(GtkGLArea* self, GdkGLContext* context, gpointer user_data);

DummyWindow::DummyWindow()
    : window(gtk_window_new(GTK_WINDOW_TOPLEVEL)), gl_area(gtk_gl_area_new()) {
    gtk_container_add(GTK_CONTAINER(window), gl_area);

    g_signal_connect(window, "realize", G_CALLBACK(realize_window), this);
    g_signal_connect(gl_area, "realize", G_CALLBACK(realize), this);
    g_signal_connect(gl_area, "render", G_CALLBACK(render), this);
}

void DummyWindow::show() {
    gtk_widget_show_all(window);
}

static void realize_window(GtkWidget* self, gpointer user_data) {
    DummyWindow* dummy_window = static_cast<DummyWindow*>(user_data);

    std::cerr << "realize_window\n";

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

    if (dummy_window->egl_context == EGL_NO_CONTEXT) {
        std::cerr << "Failed to create EGL context\n";
    }

    // dummy_window->egl_context = nullptr;
    // dummy_window->egl_context = (EGLContext*)eglGetCurrentContext();
}

static void realize(GtkWidget* self, gpointer user_data) {
    DummyWindow* dummy_window = static_cast<DummyWindow*>(user_data);

    gtk_gl_area_make_current(GTK_GL_AREA(self));
    if (gtk_gl_area_get_error(GTK_GL_AREA(self)) != nullptr) return;

    dummy_window->gl_context = gtk_gl_area_get_context(GTK_GL_AREA(self));

    std::cerr << "realize\n";

    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD.\n";
    }

    glClearColor(0, 1, 0, 1);
}

static gboolean render(GtkGLArea* self, GdkGLContext* context, gpointer user_data) {
    DummyWindow* dummy_window = static_cast<DummyWindow*>(user_data);

    // eglMakeCurrent(dummy_window->egl_display, dummy_window->egl_surface,
    // dummy_window->egl_surface,
    //                dummy_window->egl_context);

    glClear(GL_COLOR_BUFFER_BIT);
    std::cerr << "draw " << dummy_window->temp << '\n';

    eglSwapBuffers(dummy_window->egl_display, dummy_window->egl_surface);
    return false;
}
