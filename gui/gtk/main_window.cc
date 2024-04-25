#include "main_window.h"
#include <glad/glad.h>
#include <iostream>

// GtkWindow callbacks.
static gboolean key_press_event(GtkWidget* self, GdkEventKey* event, gpointer user_data);
static void destroy(GtkWidget* self, gpointer user_data);
// GLArea callbacks.
static GdkGLContext* create_context(GtkGLArea* self, gpointer user_data);
static void realize(GtkWidget* self, gpointer user_data);
static gboolean render(GtkGLArea* self, GdkGLContext* context, gpointer user_data);
static void resize(GtkGLArea* self, gint width, gint height, gpointer user_data);
static gboolean scroll_event(GtkWidget* self, GdkEventScroll* event, gpointer user_data);
static gboolean button_press_event(GtkWidget* self, GdkEventButton* event, gpointer user_data);
static gboolean motion_notify_event(GtkWidget* self, GdkEventMotion* event, gpointer user_data);

// https://github.com/ToshioCP/Gtk4-tutorial/blob/main/gfm/sec17.md#menu-and-action
static void quit_callback(GSimpleAction* action, GVariant* parameter, gpointer app) {
    g_application_quit(G_APPLICATION(app));
}

MainWindow::MainWindow(GtkApplication* gtk_app, App::Window* app_window)
    : window{gtk_application_window_new(gtk_app)}, gl_area{gtk_gl_area_new()},
      app_window{app_window} {
    gtk_window_set_title(GTK_WINDOW(window), "Simple Text");
    gtk_container_add(GTK_CONTAINER(window), gl_area);

    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(key_press_event), this);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), this);

    g_signal_connect(gl_area, "create-context", G_CALLBACK(create_context), nullptr);
    g_signal_connect(gl_area, "realize", G_CALLBACK(realize), this);
    g_signal_connect(gl_area, "render", G_CALLBACK(render), this);
    g_signal_connect(gl_area, "resize", G_CALLBACK(resize), this);
    gtk_widget_add_events(gl_area, GDK_SMOOTH_SCROLL_MASK);
    g_signal_connect(gl_area, "scroll-event", G_CALLBACK(scroll_event), this);
    gtk_widget_add_events(gl_area, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    g_signal_connect(gl_area, "button-press-event", G_CALLBACK(button_press_event), this);
    gtk_widget_add_events(gl_area, GDK_BUTTON1_MOTION_MASK);
    g_signal_connect(gl_area, "motion-notify-event", G_CALLBACK(motion_notify_event), this);

    // Add menu bar.
    {
        GMenu* menu_bar = g_menu_new();
        GMenu* file_menu = g_menu_new();
        GMenuItem* quit_menu_item = g_menu_item_new("Quit", "app.quit");

        g_menu_append_submenu(menu_bar, "File", G_MENU_MODEL(file_menu));
        g_menu_append_item(file_menu, quit_menu_item);
        gtk_application_set_menubar(GTK_APPLICATION(gtk_app), G_MENU_MODEL(menu_bar));
        gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(window), true);

        GSimpleAction* quit_action = g_simple_action_new("quit", nullptr);
        g_action_map_add_action(G_ACTION_MAP(gtk_app), G_ACTION(quit_action));
        g_signal_connect(quit_action, "activate", G_CALLBACK(quit_callback), gtk_app);
    }

    // gtk_window_maximize(GTK_WINDOW(window));
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 600);
}

void MainWindow::show() {
    gtk_widget_show_all(window);
}

void MainWindow::close() {
    gtk_window_close(GTK_WINDOW(window));
}

void MainWindow::redraw() {
    gtk_widget_queue_draw(window);
}

int MainWindow::width() {
    return gtk_widget_get_allocated_width(gl_area);
}

int MainWindow::height() {
    return gtk_widget_get_allocated_height(gl_area);
}

int MainWindow::scaleFactor() {
    return gtk_widget_get_scale_factor(window);
}

static app::Key GetKey(guint vk) {
    static constexpr struct {
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

static gboolean key_press_event(GtkWidget* self, GdkEventKey* event, gpointer user_data) {
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

    MainWindow* main_window = static_cast<MainWindow*>(user_data);
    main_window->app_window->onKeyDown(key, modifiers);

    return true;
}

static void destroy(GtkWidget* self, gpointer user_data) {
    MainWindow* main_window = static_cast<MainWindow*>(user_data);
    main_window->app_window->onClose();
}

static GdkGLContext* create_context(GtkGLArea* self, gpointer user_data) {
    GError* error = nullptr;
    GdkGLContext* context =
        gdk_window_create_gl_context(gtk_widget_get_parent_window(GTK_WIDGET(self)), &error);
    return context;
}

static void realize(GtkWidget* self, gpointer user_data) {
    gtk_gl_area_make_current(GTK_GL_AREA(self));
    if (gtk_gl_area_get_error(GTK_GL_AREA(self)) != nullptr) return;

    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD\n";
    }

    int scale_factor = gtk_widget_get_scale_factor(self);
    int scaled_width = gtk_widget_get_allocated_width(self) * scale_factor;
    int scaled_height = gtk_widget_get_allocated_height(self) * scale_factor;

    MainWindow* main_window = static_cast<MainWindow*>(user_data);
    main_window->app_window->onOpenGLActivate(scaled_width, scaled_height);
}

static gboolean render(GtkGLArea* self, GdkGLContext* context, gpointer user_data) {
    gtk_gl_area_make_current(self);

    MainWindow* main_window = static_cast<MainWindow*>(user_data);
    main_window->app_window->onDraw();

    // Draw commands are flushed after returning.
    return true;
}

static void resize(GtkGLArea* self, gint width, gint height, gpointer user_data) {
    gtk_gl_area_make_current(self);

    MainWindow* main_window = static_cast<MainWindow*>(user_data);
    main_window->app_window->onResize(width, height);
}

static gboolean scroll_event(GtkWidget* self, GdkEventScroll* event, gpointer user_data) {
    gtk_gl_area_make_current(GTK_GL_AREA(self));

    double dx, dy;
    gdk_event_get_scroll_deltas((GdkEvent*)event, &dx, &dy);

    if (gdk_device_get_source(event->device) == GDK_SOURCE_MOUSE) {
        dx *= 32;
        dy *= 32;
    }

    MainWindow* main_window = static_cast<MainWindow*>(user_data);
    main_window->app_window->onScroll(dx, dy);

    return true;
}

static gboolean button_press_event(GtkWidget* self, GdkEventButton* event, gpointer user_data) {
    if (event->type == GDK_BUTTON_PRESS) {
        gdouble mouse_x = event->x;
        gdouble mouse_y = event->y;

        int scale_factor = gtk_widget_get_scale_factor(self);
        float scaled_mouse_x = mouse_x * scale_factor;
        float scaled_mouse_y = mouse_y * scale_factor;

        MainWindow* main_window = static_cast<MainWindow*>(user_data);
        main_window->app_window->onLeftMouseDown(scaled_mouse_x, scaled_mouse_y);
    }
    return true;
}

static gboolean motion_notify_event(GtkWidget* self, GdkEventMotion* event, gpointer user_data) {
    if (event->type == GDK_MOTION_NOTIFY) {
        gdouble mouse_x = event->x;
        gdouble mouse_y = event->y;

        int scale_factor = gtk_widget_get_scale_factor(self);
        float scaled_mouse_x = mouse_x * scale_factor;
        float scaled_mouse_y = mouse_y * scale_factor;

        MainWindow* main_window = static_cast<MainWindow*>(user_data);
        main_window->app_window->onLeftMouseDrag(scaled_mouse_x, scaled_mouse_y);
    }
    return true;
}
