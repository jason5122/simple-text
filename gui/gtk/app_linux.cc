#include "gui/app.h"
#include "gui/gtk/main_window.h"
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

void App::quit() {
    g_application_quit(G_APPLICATION(pimpl->app));
}

App::~App() {
    g_object_unref(pimpl->app);
}

class App::Window::impl {
public:
    impl(GtkApplication* app, App::Window* app_window) : main_window(app, app_window) {}

    MainWindow main_window;
};

App::Window::Window(App& parent, int width, int height)
    : pimpl{new impl{parent.pimpl->app, this}}, parent(parent) {}

App::Window::~Window() {}

void App::Window::show() {
    pimpl->main_window.show();
}

void App::Window::close() {
    pimpl->main_window.close();
}

void App::Window::redraw() {
    pimpl->main_window.redraw();
}

int App::Window::width() {
    return pimpl->main_window.width();
}

int App::Window::height() {
    return pimpl->main_window.height();
}

int App::Window::scaleFactor() {
    return pimpl->main_window.scaleFactor();
}

bool App::Window::isDarkMode() {
    return pimpl->main_window.isDarkMode();
}
