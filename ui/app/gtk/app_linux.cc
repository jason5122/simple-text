#include "ui/app/app.h"
#include <glad/glad.h>
#include <gtk/gtk.h>

class App::impl {
public:
    GtkApplication* app;
};

static gboolean my_keypress_function(GtkWidget* widget, GdkEventKey* event, gpointer data) {
    if (event->keyval == GDK_KEY_w && event->state & GDK_CONTROL_MASK) {
        gtk_window_close(GTK_WINDOW(widget));
        return true;
    }
    if (event->keyval == GDK_KEY_q && event->state & GDK_CONTROL_MASK) {
        g_application_quit(G_APPLICATION(data));
        return true;
    }
    return false;
}

static void activate(GtkApplication* gtk_app, gpointer p_app) {
    App* app = static_cast<App*>(p_app);
    app->onActivate();
}

static gboolean render(GtkGLArea* self, GdkGLContext* context, gpointer user_data) {
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw commands are flushed after returning.
    return true;
}

static void realize(GtkWidget* widget) {
    gtk_gl_area_make_current(GTK_GL_AREA(widget));
    if (gtk_gl_area_get_error(GTK_GL_AREA(widget)) != nullptr) return;

    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD\n";
    }

    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glClearColor(1.0, 0.0, 0.0, 1.0);
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

void App::createNewWindow() {
    GtkWidget* window = gtk_application_window_new(pimpl->app);
    gtk_window_set_title(GTK_WINDOW(window), "Simple Text");

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, false);
    gtk_box_set_spacing(GTK_BOX(box), 6);
    gtk_container_add(GTK_CONTAINER(window), box);

    GtkWidget* gl_area = gtk_gl_area_new();
    gtk_box_pack_start(GTK_BOX(box), gl_area, 1, 1, 0);
    g_signal_connect(gl_area, "render", G_CALLBACK(render), nullptr);
    g_signal_connect(gl_area, "realize", G_CALLBACK(realize), nullptr);

    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(my_keypress_function),
                     pimpl->app);

    // gtk_window_maximize(GTK_WINDOW(window));
    // TODO: Set default window size without magic numbers.
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 400);

    gtk_widget_show_all(window);
}

App::~App() {
    g_object_unref(pimpl->app);
}
