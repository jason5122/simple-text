#include "gdk/gdkkeysyms.h"
#include "main_window.h"
#include <cmath>

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

#include <format>
#include <iostream>

namespace app {

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
static gboolean
crossing_notify_event(GtkWidget* self, GdkEventCrossing* event, gpointer user_data);
static void settings_changed_signal_cb(GDBusProxy* proxy,
                                       gchar* sender_name,
                                       gchar* signal_name,
                                       GVariant* parameters,
                                       gpointer user_data);

// https://github.com/ToshioCP/Gtk4-tutorial/blob/main/gfm/sec17.md#menu-and-action
static void quit_callback(GSimpleAction* action, GVariant* parameter, gpointer app) {
    std::cerr << "quit callback\n";
    g_application_quit(G_APPLICATION(app));
}

MainWindow::MainWindow(GtkApplication* gtk_app, gui::Window* app_window)
    : window{gtk_application_window_new(gtk_app)},
      gl_area{gtk_gl_area_new()},
      app_window{app_window} {
    gtk_window_set_title(GTK_WINDOW(window), "Simple Text");
    gtk_container_add(GTK_CONTAINER(window), gl_area);

    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(key_press_event), this);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), this);

    g_signal_connect(gl_area, "create-context", G_CALLBACK(create_context), this);
    g_signal_connect(gl_area, "realize", G_CALLBACK(realize), this);
    g_signal_connect(gl_area, "render", G_CALLBACK(render), this);
    g_signal_connect(gl_area, "resize", G_CALLBACK(resize), this);
    gtk_widget_add_events(gl_area, GDK_SMOOTH_SCROLL_MASK);
    g_signal_connect(gl_area, "scroll-event", G_CALLBACK(scroll_event), this);
    gtk_widget_add_events(gl_area, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    g_signal_connect(gl_area, "button-press-event", G_CALLBACK(button_press_event), this);
    gtk_widget_add_events(gl_area, GDK_BUTTON1_MOTION_MASK);
    g_signal_connect(gl_area, "motion-notify-event", G_CALLBACK(motion_notify_event), this);
    gtk_widget_add_events(gl_area, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
    g_signal_connect(gl_area, "enter-notify-event", G_CALLBACK(crossing_notify_event), this);
    g_signal_connect(gl_area, "leave-notify-event", G_CALLBACK(crossing_notify_event), this);

    // Add menu bar.
    {
        GMenu* menu_bar = g_menu_new();
        GMenu* file_menu = g_menu_new();
        GMenuItem* quit_menu_item = g_menu_item_new("Quit", "app.quit");

        g_menu_append_submenu(menu_bar, "File", G_MENU_MODEL(file_menu));
        g_menu_append_item(file_menu, quit_menu_item);
        gtk_application_set_menubar(gtk_app, G_MENU_MODEL(menu_bar));

        gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(window), true);

        // GSimpleAction* quit_action = g_simple_action_new("quit", nullptr);
        // g_action_map_add_action(G_ACTION_MAP(gtk_app), G_ACTION(quit_action));
        // g_signal_connect(quit_action, "activate", G_CALLBACK(quit_callback), gtk_app);

        const GActionEntry entries[] = {{"quit", quit_callback}};
        g_action_map_add_action_entries(G_ACTION_MAP(gtk_app), entries, G_N_ELEMENTS(entries),
                                        gtk_app);

        const gchar* quit_accels[2] = {"<Ctrl>q", NULL};
        gtk_application_set_accels_for_action(gtk_app, "app.quit", quit_accels);
    }

    // gtk_window_maximize(GTK_WINDOW(window));
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 600);

    // Use DBus to query and listen for light/dark theme changes.
    GError* error = nullptr;
    dbus_settings_proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, nullptr, "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop", "org.freedesktop.portal.Settings", nullptr, &error);

    g_signal_connect(dbus_settings_proxy, "g-signal", G_CALLBACK(settings_changed_signal_cb),
                     this);

    if (isDarkMode()) {
        g_object_set(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", true,
                     nullptr);
    }
}

MainWindow::~MainWindow() {
    std::cerr << "~MainWindow\n";
    // TODO: See if we need `g_object_unref` for any other object.
    g_object_unref(dbus_settings_proxy);
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

bool MainWindow::isDarkMode() {
    GError* error = nullptr;
    GVariant* variant =
        g_dbus_proxy_call_sync(dbus_settings_proxy, "Read",
                               g_variant_new("(ss)", "org.freedesktop.appearance", "color-scheme"),
                               G_DBUS_CALL_FLAGS_NONE, 3000, nullptr, &error);

    variant = g_variant_get_child_value(variant, 0);
    while (variant && g_variant_is_of_type(variant, G_VARIANT_TYPE_VARIANT)) {
        // Unbox the return value.
        variant = g_variant_get_variant(variant);
    }

    // 0: default
    // 1: dark
    // 2: light
    return g_variant_get_uint32(variant) == 1;
}

void MainWindow::setTitle(const std::string& title) {
    gtk_window_set_title(GTK_WINDOW(window), &title[0]);
}

static inline gui::Key GetKey(guint vk) {
    static constexpr struct {
        guint fVK;
        gui::Key fKey;
    } gPair[] = {
        {GDK_KEY_A, gui::Key::kA},
        {GDK_KEY_B, gui::Key::kB},
        {GDK_KEY_C, gui::Key::kC},
        {GDK_KEY_D, gui::Key::kD},
        {GDK_KEY_E, gui::Key::kE},
        {GDK_KEY_F, gui::Key::kF},
        {GDK_KEY_G, gui::Key::kG},
        {GDK_KEY_H, gui::Key::kH},
        {GDK_KEY_I, gui::Key::kI},
        {GDK_KEY_J, gui::Key::kJ},
        {GDK_KEY_K, gui::Key::kK},
        {GDK_KEY_L, gui::Key::kL},
        {GDK_KEY_M, gui::Key::kM},
        {GDK_KEY_N, gui::Key::kN},
        {GDK_KEY_O, gui::Key::kO},
        {GDK_KEY_P, gui::Key::kP},
        {GDK_KEY_Q, gui::Key::kQ},
        {GDK_KEY_R, gui::Key::kR},
        {GDK_KEY_S, gui::Key::kS},
        {GDK_KEY_T, gui::Key::kT},
        {GDK_KEY_U, gui::Key::kU},
        {GDK_KEY_V, gui::Key::kV},
        {GDK_KEY_W, gui::Key::kW},
        {GDK_KEY_X, gui::Key::kX},
        {GDK_KEY_Y, gui::Key::kY},
        {GDK_KEY_Z, gui::Key::kZ},
        {GDK_KEY_0, gui::Key::k0},
        {GDK_KEY_1, gui::Key::k1},
        {GDK_KEY_2, gui::Key::k2},
        {GDK_KEY_3, gui::Key::k3},
        {GDK_KEY_4, gui::Key::k4},
        {GDK_KEY_5, gui::Key::k5},
        {GDK_KEY_6, gui::Key::k6},
        {GDK_KEY_7, gui::Key::k7},
        {GDK_KEY_8, gui::Key::k8},
        {GDK_KEY_9, gui::Key::k9},
        {GDK_KEY_Return, gui::Key::kEnter},
        {GDK_KEY_BackSpace, gui::Key::kBackspace},
        {GDK_KEY_Left, gui::Key::kLeftArrow},
        {GDK_KEY_Right, gui::Key::kRightArrow},
        {GDK_KEY_Down, gui::Key::kDownArrow},
        {GDK_KEY_Up, gui::Key::kUpArrow},
    };

    for (size_t i = 0; i < std::size(gPair); i++) {
        if (gPair[i].fVK == vk) {
            return gPair[i].fKey;
        }
    }

    return gui::Key::kNone;
}

static inline gui::ModifierKey GetModifiers(guint state) {
    gui::ModifierKey modifiers = gui::ModifierKey::kNone;
    if (state & GDK_SHIFT_MASK) {
        modifiers |= gui::ModifierKey::kShift;
    }
    if (state & GDK_CONTROL_MASK) {
        modifiers |= gui::ModifierKey::kControl;
    }
    if (state & GDK_MOD1_MASK) {
        modifiers |= gui::ModifierKey::kAlt;
    }
    if (state & GDK_SUPER_MASK) {
        modifiers |= gui::ModifierKey::kSuper;
    }
    return modifiers;
}

static gboolean key_press_event(GtkWidget* self, GdkEventKey* event, gpointer user_data) {
    gui::Key key = GetKey(gdk_keyval_to_upper(event->keyval));
    gui::ModifierKey modifiers = GetModifiers(event->state);

    MainWindow* main_window = static_cast<MainWindow*>(user_data);
    bool handled = main_window->app_window->onKeyDown(key, modifiers);

    if (!handled) {
        guint32 codepoint = gdk_keyval_to_unicode(event->keyval);

        char* str;
        size_t len = grapheme_encode_utf8(codepoint, nullptr, 0);
        str = new char[len + 1];
        grapheme_encode_utf8(codepoint, str, len);
        str[len] = '\0';

        main_window->app_window->onInsertText(str);
        free(str);
    }

    // TODO: Determine when to propagate and when not to.
    std::cerr << "key press\n";
    return false;
}

static void destroy(GtkWidget* self, gpointer user_data) {
    MainWindow* main_window = static_cast<MainWindow*>(user_data);
    main_window->app_window->onClose();
}

static GdkGLContext* create_context(GtkGLArea* self, gpointer user_data) {
    MainWindow* main_window = static_cast<MainWindow*>(user_data);

    GError* error = nullptr;
    // GdkWindow* gdk_window = gtk_widget_get_window(GTK_WIDGET(self));
    GdkWindow* gdk_window = gtk_widget_get_window(main_window->window);

    // if (!MainWindow::context) {
    //     GdkGLContext* new_context = gdk_window_create_gl_context(gdk_window, &error);
    //     MainWindow::context = g_object_ref(new_context);
    // }
    // return MainWindow::context;
    return gdk_window_create_gl_context(gdk_window, &error);
}

static void realize(GtkWidget* self, gpointer user_data) {
    gtk_gl_area_make_current(GTK_GL_AREA(self));
    if (gtk_gl_area_get_error(GTK_GL_AREA(self)) != nullptr) return;

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

    // TODO: For debugging; remove this.
    // main_window->app_window->stopLaunchTimer();

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

    double delta_x, delta_y;
    gdk_event_get_scroll_deltas((GdkEvent*)event, &delta_x, &delta_y);

    int dx = std::round(delta_x);
    int dy = std::round(delta_y);
    if (gdk_device_get_source(event->device) == GDK_SOURCE_MOUSE) {
        dx *= 32;
        dy *= 32;
    }

    MainWindow* main_window = static_cast<MainWindow*>(user_data);
    main_window->app_window->onScroll(dx, dy);

    return true;
}

static gboolean button_press_event(GtkWidget* self, GdkEventButton* event, gpointer user_data) {
    if (event->type == GDK_BUTTON_PRESS || event->type == GDK_2BUTTON_PRESS ||
        event->type == GDK_3BUTTON_PRESS) {
        int mouse_x = std::round(event->x);
        int mouse_y = std::round(event->y);

        int scale_factor = gtk_widget_get_scale_factor(self);
        int scaled_mouse_x = mouse_x * scale_factor;
        int scaled_mouse_y = mouse_y * scale_factor;

        gui::ModifierKey modifiers = GetModifiers(event->state);

        gui::ClickType click_type = gui::ClickType::kSingleClick;
        if (event->type == GDK_2BUTTON_PRESS) {
            click_type = gui::ClickType::kDoubleClick;
        } else if (event->type == GDK_3BUTTON_PRESS) {
            click_type = gui::ClickType::kTripleClick;
        }

        MainWindow* main_window = static_cast<MainWindow*>(user_data);
        main_window->app_window->onLeftMouseDown(scaled_mouse_x, scaled_mouse_y, modifiers,
                                                 click_type);
    }
    return true;
}

static gboolean motion_notify_event(GtkWidget* self, GdkEventMotion* event, gpointer user_data) {
    if (event->type == GDK_MOTION_NOTIFY) {
        int mouse_x = std::round(event->x);
        int mouse_y = std::round(event->y);

        int scale_factor = gtk_widget_get_scale_factor(self);
        int scaled_mouse_x = mouse_x * scale_factor;
        int scaled_mouse_y = mouse_y * scale_factor;

        gui::ModifierKey modifiers = GetModifiers(event->state);

        MainWindow* main_window = static_cast<MainWindow*>(user_data);
        main_window->app_window->onLeftMouseDrag(scaled_mouse_x, scaled_mouse_y, modifiers);
    }
    return true;
}

static gboolean
crossing_notify_event(GtkWidget* self, GdkEventCrossing* event, gpointer user_data) {
    GdkDisplay* display = gtk_widget_get_display(self);

    // TODO: Create `GdkCursor` objects once on realize instead of re-creating them each time.
    GdkCursor* cursor;
    if (event->type == GDK_ENTER_NOTIFY) {
        cursor = gdk_cursor_new_from_name(display, "text");
    }
    if (event->type == GDK_LEAVE_NOTIFY) {
        cursor = gdk_cursor_new_from_name(display, "default");
    }
    gdk_window_set_cursor(gtk_widget_get_window(self), cursor);
    g_object_unref(cursor);
    return true;
}

static void settings_changed_signal_cb(GDBusProxy* proxy,
                                       gchar* sender_name,
                                       gchar* signal_name,
                                       GVariant* parameters,
                                       gpointer user_data) {
    MainWindow* main_window = static_cast<MainWindow*>(user_data);

    GVariant* ns = g_variant_get_child_value(parameters, 0);
    GVariant* key = g_variant_get_child_value(parameters, 1);

    gsize len = 0;
    std::string ns_str = g_variant_get_string(ns, &len);
    std::string key_str = g_variant_get_string(key, &len);
    if (ns_str == "org.freedesktop.appearance" && key_str == "color-scheme") {
        // TODO: See if this is optimal. Also consider combining this with the other check during
        // initialization.
        if (main_window->isDarkMode()) {
            g_object_set(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", true,
                         nullptr);
        } else {
            g_object_set(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", false,
                         nullptr);
        }

        main_window->app_window->onDarkModeToggle();
    }
}

}
