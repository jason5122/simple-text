#include "ui/app.h"
#include "ui/gtk/main_window.h"
#include <glad/glad.h>
#include <gtk/gtk.h>

static void activate(GtkApplication* gtk_app, gpointer p_app) {
    App* app = static_cast<App*>(p_app);
    app->onLaunch();
}

class App::impl {
public:
    GtkApplication* app;
};

App::App() : pimpl{new impl{}} {
#if GLIB_CHECK_VERSION(2, 74, 0)
    GApplicationFlags flags = G_APPLICATION_DEFAULT_FLAGS;
#else
    GApplicationFlags flags = G_APPLICATION_FLAGS_NONE;
#endif
    pimpl->app = gtk_application_new("com.jason.simple-text", flags);
    g_signal_connect(pimpl->app, "activate", G_CALLBACK(activate), this);
}

void App::run() {
    g_application_run(G_APPLICATION(pimpl->app), 0, NULL);
}

App::~App() {
    g_object_unref(pimpl->app);
}

class App::Window::impl {
public:
    MainWindow* main_window;
};

App::Window::Window(App& parent, int width, int height) : pimpl{new impl{}}, parent(parent) {
    pimpl->main_window = new MainWindow(parent.pimpl->app, this);
}

App::Window::~Window() {}

void App::Window::show() {
    pimpl->main_window->show();
}

void App::Window::close() {
    pimpl->main_window->close();
}

void App::Window::redraw() {
    pimpl->main_window->redraw();
}
