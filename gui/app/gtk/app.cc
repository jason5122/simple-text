#include "gui/app/app.h"

#include "gui/app/gtk/impl_gtk.h"

#include <gtk/gtk.h>

namespace gui {

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
    g_setenv("GDK_DEBUG", "gl-prefer-gl", 1);
    // TODO: Disable Cairo when not testing with emulator.
    //       We enable Cairo since OpenGL has a 1-2 second startup delay on emulator.
    // g_setenv("GSK_RENDERER", "cairo", 1);

    pimpl->app = gtk_application_new("com.jason.simple-text", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(pimpl->app, "activate", G_CALLBACK(activate), this);
}

App::~App() {
    g_object_unref(pimpl->app);
}

void App::run() {
    g_application_run(G_APPLICATION(pimpl->app), 0, NULL);
}

void App::quit() {
    g_application_quit(G_APPLICATION(pimpl->app));
}

// TODO: Implement this.
std::string App::getClipboardString() {
    return "";
}

// TODO: Implement this.
void App::setClipboardString(const std::string& str8) {}

}  // namespace gui
