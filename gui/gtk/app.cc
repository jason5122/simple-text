#include "gui/app.h"
#include "gui/gtk/pimpl_linux.h"
#include <glad/glad.h>
#include <gtk/gtk.h>

namespace gui {

static void activate(GtkApplication* gtk_app, gpointer p_app) {
    App* app = static_cast<App*>(p_app);
    app->onLaunch();
}

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

}
