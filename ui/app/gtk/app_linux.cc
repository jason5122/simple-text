#include "ui/app/app.h"
#include "ui/app/gtk/main_window.h"
#include <glad/glad.h>
#include <gtk/gtk.h>
#include <iostream>

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

static app::Key GetKey(guint vk) {
    static const struct {
        guint fVK;
        app::Key fKey;
    } gPair[] = {
        {GDK_KEY_A, app::Key::kA},
        {GDK_KEY_B, app::Key::kB},
        {GDK_KEY_C, app::Key::kC},
        // TODO: Implement the rest.
        {GDK_KEY_N, app::Key::kN},
        {GDK_KEY_Q, app::Key::kQ},
        {GDK_KEY_W, app::Key::kW},
    };

    for (size_t i = 0; i < std::size(gPair); i++) {
        if (gPair[i].fVK == vk) {
            return gPair[i].fKey;
        }
    }

    return app::Key::kNone;
}

static gboolean key_press_event(GtkWidget* widget, GdkEventKey* event, gpointer p_app_window) {
    App::Window* app_window = static_cast<App::Window*>(p_app_window);

    app::Key key = GetKey(gdk_keyval_to_upper(event->keyval));

    app::ModifierKey modifiers = app::ModifierKey::kNone;
    if (event->state & GDK_SHIFT_MASK) {
        modifiers |= app::ModifierKey::kShift;
    }
    if (event->state & GDK_CONTROL_MASK) {
        modifiers |= app::ModifierKey::kControl;
    }
    if (event->state & GDK_MOD1_MASK) {
        modifiers |= app::ModifierKey::kAlt;
    }
    if (event->state & GDK_SUPER_MASK) {
        modifiers |= app::ModifierKey::kSuper;
    }

    app_window->onKeyDown(key, modifiers);

    return true;
}

static void activate(GtkApplication* gtk_app, gpointer p_app) {
    App* app = static_cast<App*>(p_app);
    app->onLaunch();
}

static void realize(GtkWidget* widget, gpointer p_app_window) {
    App::Window* app_window = static_cast<App::Window*>(p_app_window);

    gtk_gl_area_make_current(GTK_GL_AREA(widget));
    if (gtk_gl_area_get_error(GTK_GL_AREA(widget)) != nullptr) return;

    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD\n";
    }

    int scale_factor = gtk_widget_get_scale_factor(widget);
    int scaled_width = gtk_widget_get_allocated_width(widget) * scale_factor;
    int scaled_height = gtk_widget_get_allocated_height(widget) * scale_factor;

    app_window->onOpenGLActivate(scaled_width, scaled_height);
}

static gboolean render(GtkGLArea* self, GdkGLContext* context, gpointer p_app_window) {
    App::Window* app_window = static_cast<App::Window*>(p_app_window);

    gtk_gl_area_make_current(self);

    app_window->onDraw();

    // Draw commands are flushed after returning.
    return true;
}

static void resize(GtkGLArea* self, gint width, gint height, gpointer p_app_window) {
    App::Window* app_window = static_cast<App::Window*>(p_app_window);

    gtk_gl_area_make_current(self);

    app_window->onResize(width, height);
}

static gboolean scroll_event(GtkWidget* widget, GdkEventScroll* event, gpointer p_app_window) {
    App::Window* app_window = static_cast<App::Window*>(p_app_window);

    gtk_gl_area_make_current(GTK_GL_AREA(widget));

    double dx, dy;
    gdk_event_get_scroll_deltas((GdkEvent*)event, &dx, &dy);

    if (gdk_device_get_source(event->device) == GDK_SOURCE_MOUSE) {
        std::cerr << "mouse\n";
        dx *= 32;
        dy *= 32;
    }

    app_window->onScroll(dx, dy);

    return true;
}

static gboolean button_event(GtkWidget* widget, GdkEventButton* event, gpointer p_app_window) {
    App::Window* app_window = static_cast<App::Window*>(p_app_window);

    if (event->type == GDK_BUTTON_PRESS) {
        gdouble mouse_x = event->x;
        gdouble mouse_y = event->y;

        int scale_factor = gtk_widget_get_scale_factor(widget);
        float scaled_mouse_x = mouse_x * scale_factor;
        float scaled_mouse_y = mouse_y * scale_factor;

        app_window->onLeftMouseDown(scaled_mouse_x, scaled_mouse_y);
    }
    return true;
}

static gboolean motion_event(GtkWidget* widget, GdkEventMotion* event, gpointer p_app_window) {
    App::Window* app_window = static_cast<App::Window*>(p_app_window);

    if (event->type == GDK_MOTION_NOTIFY) {
        gdouble mouse_x = event->x;
        gdouble mouse_y = event->y;

        int scale_factor = gtk_widget_get_scale_factor(widget);
        float scaled_mouse_x = mouse_x * scale_factor;
        float scaled_mouse_y = mouse_y * scale_factor;

        app_window->onLeftMouseDrag(scaled_mouse_x, scaled_mouse_y);
    }
    return true;
}

// https://github.com/ToshioCP/Gtk4-tutorial/blob/main/gfm/sec17.md#menu-and-action
static void quit_callback(GSimpleAction* action, GVariant* parameter, gpointer app) {
    g_application_quit(G_APPLICATION(app));
}

static GdkGLContext* create_context(GtkGLArea* self, gpointer p_context) {
    return static_cast<GdkGLContext*>(p_context);
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

// This isn't required for GTK 3.
void App::incrementWindowCount() {}

App::~App() {
    g_object_unref(pimpl->app);
}

class App::Window::impl {
public:
    MainWindow* main_window;
    GtkWidget* window_widget;
};

App::Window::Window(App& parent, int width, int height) : pimpl{new impl{}}, parent(parent) {
    pimpl->main_window = new MainWindow(parent.pimpl->app, this);

    // pimpl->window_widget = gtk_application_window_new(parent.pimpl->app);
    // gtk_window_set_title(GTK_WINDOW(pimpl->window_widget), "Simple Text");

    // gtk_widget_add_events(pimpl->window_widget, GDK_KEY_PRESS_MASK);
    // g_signal_connect(G_OBJECT(pimpl->window_widget), "key_press_event",
    //                  G_CALLBACK(key_press_event), this);

    // // FIXME: Test out realizing widget after attaching it to a window.
    // {
    //     GtkWidget* dummy_gl_area = gtk_gl_area_new();
    //     // gtk_widget_realize(dummy_gl_area);
    //     GdkGLContext* context = gtk_gl_area_get_context(GTK_GL_AREA(dummy_gl_area));

    //     if (context == nullptr) {
    //         std::cerr << "GdkGLContext is nullptr\n";
    //     }
    // }

    // GtkWidget* gl_area = gtk_gl_area_new();
    // // g_signal_connect(gl_area, "create-context", G_CALLBACK(create_context), context);
    // g_signal_connect(gl_area, "realize", G_CALLBACK(realize), this);
    // g_signal_connect(gl_area, "render", G_CALLBACK(render), this);
    // g_signal_connect(gl_area, "resize", G_CALLBACK(resize), this);
    // gtk_container_add(GTK_CONTAINER(pimpl->window_widget), gl_area);

    // gtk_widget_add_events(gl_area, GDK_SMOOTH_SCROLL_MASK);
    // g_signal_connect(gl_area, "scroll-event", G_CALLBACK(scroll_event), this);

    // gtk_widget_add_events(gl_area, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    // g_signal_connect(G_OBJECT(gl_area), "button-press-event", G_CALLBACK(button_event), this);

    // gtk_widget_add_events(gl_area, GDK_BUTTON1_MOTION_MASK);
    // g_signal_connect(G_OBJECT(gl_area), "motion-notify-event", G_CALLBACK(motion_event), this);

    // // Add menu bar.
    // {
    //     GMenu* menu_bar = g_menu_new();
    //     GMenu* file_menu = g_menu_new();
    //     GMenuItem* quit_menu_item = g_menu_item_new("Quit", "app.quit");

    //     g_menu_append_submenu(menu_bar, "File", G_MENU_MODEL(file_menu));
    //     g_menu_append_item(file_menu, quit_menu_item);
    //     gtk_application_set_menubar(GTK_APPLICATION(parent.pimpl->app), G_MENU_MODEL(menu_bar));
    //     gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(pimpl->window_widget),
    //                                             true);

    //     GSimpleAction* quit_action = g_simple_action_new("quit", nullptr);
    //     g_action_map_add_action(G_ACTION_MAP(parent.pimpl->app), G_ACTION(quit_action));
    //     g_signal_connect(quit_action, "activate", G_CALLBACK(quit_callback), parent.pimpl->app);
    // }

    // // gtk_window_maximize(GTK_WINDOW(pimpl->window_widget));
    // gtk_window_set_default_size(GTK_WINDOW(pimpl->window_widget), width, height);
}

void App::Window::show() {
    // gtk_widget_show_all(pimpl->window_widget);
    pimpl->main_window->show();
}

void App::Window::close() {
    // gtk_window_close(GTK_WINDOW(pimpl->window_widget));
    pimpl->main_window->close();
}

void App::Window::redraw() {
    // gtk_widget_queue_draw(pimpl->window_widget);
}

App::Window::~Window() {}
