#include "app/app.h"
#include "app/gtk/pimpl_linux.h"
#include <gtk/gtk.h>

namespace app {

static void activate(GtkApplication* gtk_app, gpointer p_app) {
    App* app = static_cast<App*>(p_app);

    GError* error = nullptr;
    GdkDisplay* display = gdk_display_get_default();
    app->pimpl->context = gdk_display_create_gl_context(display, &error);

    gdk_gl_context_make_current(app->pimpl->context);
    app->onLaunch();
    gdk_gl_context_clear_current();
}

App::App() : pimpl{new impl{}} {
    pimpl->app = gtk_application_new("com.jason.simple-text", G_APPLICATION_DEFAULT_FLAGS);
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

}
