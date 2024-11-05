#include "main_window.h"
#include "unicode/unicode.h"
#include <cmath>

// Debug use; remove this.
#include "util/std_print.h"

namespace {

constexpr app::Key KeyFromKeyval(guint keyval);
constexpr app::ModifierKey ModifierFromState(guint state);
// TODO: Make this work with multiple modifiers.
constexpr const gchar* GtkAccelStringFromModifier(app::ModifierKey modifier);

}  // namespace

namespace app {

// GtkWindow callbacks.
static void destroy(GtkWidget* self, gpointer user_data);
// GtkGLArea callbacks.
static GdkGLContext* create_context(GtkGLArea* self, gpointer user_data);
static void realize(GtkGLArea* self, gpointer user_data);
static gboolean render(GtkGLArea* self, GdkGLContext* context, gpointer user_data);
static void resize(GtkGLArea* self, gint width, gint height, gpointer user_data);
static gboolean scroll(GtkEventControllerScroll* self, gdouble dx, gdouble dy, gpointer user_data);
static void pressed(GtkGestureClick* self, gint n_press, gdouble x, gdouble y, gpointer user_data);
static void motion(GtkEventControllerMotion* self, gdouble x, gdouble y, gpointer user_data);
static void quit_callback(GSimpleAction* action, GVariant* parameter, gpointer app);
static gboolean key_pressed(GtkEventControllerKey* self,
                            guint keyval,
                            guint keycode,
                            GdkModifierType state,
                            gpointer user_data);

MainWindow::MainWindow(GtkApplication* gtk_app, Window* app_window, GdkGLContext* context)
    : app_window{app_window},
      window{gtk_application_window_new(gtk_app)},
      gl_area{gtk_gl_area_new()} {

    gtk_window_set_title(GTK_WINDOW(window), "Simple Text");

    GtkWidget* gtk_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), gtk_box);

    gtk_widget_set_hexpand(gl_area, true);
    gtk_widget_set_vexpand(gl_area, true);
    gtk_box_append(GTK_BOX(gtk_box), gl_area);

    // Add menu bar.
    {
        GMenu* menu_bar = g_menu_new();
        GMenu* file_menu = g_menu_new();
        GMenuItem* quit_menu_item = g_menu_item_new("Quit", "app.quit");

        g_menu_append_submenu(menu_bar, "File", G_MENU_MODEL(file_menu));
        g_menu_append_item(file_menu, quit_menu_item);
        gtk_application_set_menubar(gtk_app, G_MENU_MODEL(menu_bar));

        gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(window), true);

        const GActionEntry entries[] = {{"quit", quit_callback}};
        g_action_map_add_action_entries(G_ACTION_MAP(gtk_app), entries, G_N_ELEMENTS(entries),
                                        gtk_app);

        std::string quit_accel = std::format("{}q", GtkAccelStringFromModifier(kPrimaryModifier));
        const gchar* quit_accels[2] = {quit_accel.data(), NULL};
        gtk_application_set_accels_for_action(gtk_app, "app.quit", quit_accels);
    }

    // GtkWindow callbacks.
    g_signal_connect(window, "destroy", G_CALLBACK(destroy), app_window);
    // GtkGLArea callbacks.
    g_signal_connect(gl_area, "create-context", G_CALLBACK(create_context), context);
    g_signal_connect(gl_area, "realize", G_CALLBACK(realize), app_window);
    g_signal_connect(gl_area, "render", G_CALLBACK(render), app_window);
    g_signal_connect(gl_area, "resize", G_CALLBACK(resize), app_window);

    GtkEventControllerScrollFlags scroll_flags = GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES;
    GtkEventController* scroll_event_controller = gtk_event_controller_scroll_new(scroll_flags);
    gtk_widget_add_controller(gl_area, scroll_event_controller);
    g_signal_connect(scroll_event_controller, "scroll", G_CALLBACK(scroll), this);

    GtkGesture* gesture = gtk_gesture_click_new();
    gtk_widget_add_controller(gl_area, GTK_EVENT_CONTROLLER(gesture));
    g_signal_connect(gesture, "pressed", G_CALLBACK(pressed), app_window);

    GtkEventController* motion_event_controller = gtk_event_controller_motion_new();
    gtk_widget_add_controller(gl_area, motion_event_controller);
    g_signal_connect(motion_event_controller, "motion", G_CALLBACK(motion), this);

    GtkEventController* key_event_controller = gtk_event_controller_key_new();
    gtk_widget_add_controller(window, key_event_controller);
    g_signal_connect(key_event_controller, "key-pressed", G_CALLBACK(key_pressed), app_window);

    gtk_widget_set_cursor(gl_area, gdk_cursor_new_from_name("text", nullptr));

    // gtk_window_maximize(GTK_WINDOW(window));
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 600);
}

MainWindow::~MainWindow() {
    std::println("~MainWindow");
    // TODO: See if we need `g_object_unref` for any other object.
    // g_object_unref(dbus_settings_proxy);
}

void MainWindow::show() {
    gtk_window_present(GTK_WINDOW(window));
}

void MainWindow::close() {
    gtk_window_close(GTK_WINDOW(window));
}

void MainWindow::redraw() {
    gtk_widget_queue_draw(gl_area);
}

int MainWindow::width() {
    // return gtk_widget_get_allocated_width(gl_area);
    // return gtk_widget_get_width(gl_area);
    return -1;
}

int MainWindow::height() {
    // return gtk_widget_get_allocated_height(gl_area);
    // return gtk_widget_get_height(gl_area);
    return -1;
}

int MainWindow::scaleFactor() {
    return gtk_widget_get_scale_factor(window);
}

bool MainWindow::isDarkMode() {
    // GError* error = nullptr;
    // GVariant* variant =
    //     g_dbus_proxy_call_sync(dbus_settings_proxy, "Read",
    //                            g_variant_new("(ss)", "org.freedesktop.appearance",
    //                            "color-scheme"), G_DBUS_CALL_FLAGS_NONE, 3000, nullptr, &error);

    // variant = g_variant_get_child_value(variant, 0);
    // while (variant && g_variant_is_of_type(variant, G_VARIANT_TYPE_VARIANT)) {
    //     // Unbox the return value.
    //     variant = g_variant_get_variant(variant);
    // }

    // // 0: default
    // // 1: dark
    // // 2: light
    // return g_variant_get_uint32(variant) == 1;
    return false;
}

void MainWindow::setTitle(const std::string& title) {
    gtk_window_set_title(GTK_WINDOW(window), title.data());
}

Window* MainWindow::appWindow() {
    return app_window;
}

static void destroy(GtkWidget* self, gpointer user_data) {
    Window* app_window = static_cast<Window*>(user_data);
    app_window->onClose();
}

static GdkGLContext* create_context(GtkGLArea* self, gpointer user_data) {
    GdkGLContext* context = static_cast<GdkGLContext*>(user_data);
    return g_object_ref(context);
}

static void realize(GtkGLArea* self, gpointer user_data) {
    gtk_gl_area_make_current(self);
    if (gtk_gl_area_get_error(self) != nullptr) return;

    GdkGLAPI api = gtk_gl_area_get_api(self);
    if (api == GDK_GL_API_GL) {
        std::println("GDK_GL_API_GL");
    }
    if (api == GDK_GL_API_GLES) {
        std::println("GDK_GL_API_GLES");
    }

    // FIXME: Getting the size of a widget doesn't seem to be possible during realization.
    // This is fine for now since GTK sends a resize signal once the widget is created.
    int scale_factor = gtk_widget_get_scale_factor(GTK_WIDGET(self));
    int scaled_width = gtk_widget_get_width(GTK_WIDGET(self)) * scale_factor;
    int scaled_height = gtk_widget_get_height(GTK_WIDGET(self)) * scale_factor;

    Window* app_window = static_cast<Window*>(user_data);
    app_window->onOpenGLActivate(scaled_width, scaled_height);
}

static void resize(GtkGLArea* self, gint width, gint height, gpointer user_data) {
    gtk_gl_area_make_current(self);

    Window* app_window = static_cast<Window*>(user_data);
    app_window->onResize(width, height);
}

static gboolean render(GtkGLArea* self, GdkGLContext* context, gpointer user_data) {
    gtk_gl_area_make_current(self);

    int scale_factor = gtk_widget_get_scale_factor(GTK_WIDGET(self));
    int scaled_width = gtk_widget_get_width(GTK_WIDGET(self)) * scale_factor;
    int scaled_height = gtk_widget_get_height(GTK_WIDGET(self)) * scale_factor;

    Window* app_window = static_cast<Window*>(user_data);
    app_window->onDraw(scaled_width, scaled_height);

    // Draw commands are flushed after returning.
    return true;
}

static gboolean scroll(GtkEventControllerScroll* self,
                       gdouble dx,
                       gdouble dy,
                       gpointer user_data) {
    MainWindow* main_window = static_cast<MainWindow*>(user_data);

    GtkWidget* gl_area = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(self));
    gtk_gl_area_make_current(GTK_GL_AREA(gl_area));

    GdkScrollUnit unit = gtk_event_controller_scroll_get_unit(self);
    if (unit == GDK_SCROLL_UNIT_WHEEL) {
        dx *= 32;
        dy *= 32;
    } else {
        // TODO: Figure out how to interpret scroll numbers (usually between 0.0-1.0).
        std::println("dy = {}", dy);
        // dx *= 32;
        // dy *= 32;
    }

    int mouse_x = std::round(main_window->mouse_x);
    int mouse_y = std::round(main_window->mouse_y);

    int scale_factor = gtk_widget_get_scale_factor(gl_area);
    int scaled_mouse_x = mouse_x * scale_factor;
    int scaled_mouse_y = mouse_y * scale_factor;

    int delta_x = std::round(dx);
    int delta_y = std::round(dy);
    Window* app_window = main_window->appWindow();
    app_window->onScroll(scaled_mouse_x, scaled_mouse_y, delta_x, delta_y);

    return true;
}

static void pressed(
    GtkGestureClick* self, gint n_press, gdouble x, gdouble y, gpointer user_data) {
    GtkWidget* gl_area = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(self));

    int mouse_x = std::round(x);
    int mouse_y = std::round(y);

    int scale_factor = gtk_widget_get_scale_factor(gl_area);
    int scaled_mouse_x = mouse_x * scale_factor;
    int scaled_mouse_y = mouse_y * scale_factor;

    GdkModifierType event_state =
        gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(self));
    ModifierKey modifiers = ModifierFromState(event_state);

    ClickType click_type = ClickType::kSingleClick;
    if (n_press == 2) {
        click_type = ClickType::kDoubleClick;
    } else if (n_press >= 3) {
        click_type = ClickType::kTripleClick;
    }

    Window* app_window = static_cast<Window*>(user_data);
    app_window->onLeftMouseDown(scaled_mouse_x, scaled_mouse_y, modifiers, click_type);
}

static void motion(GtkEventControllerMotion* self, gdouble x, gdouble y, gpointer user_data) {
    MainWindow* main_window = static_cast<MainWindow*>(user_data);
    main_window->mouse_x = x;
    main_window->mouse_y = y;

    GdkModifierType event_state =
        gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(self));

    // Left click drag.
    if (event_state & GDK_BUTTON1_MASK) {
        GtkWidget* gl_area = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(self));

        int mouse_x = std::round(x);
        int mouse_y = std::round(y);

        int scale_factor = gtk_widget_get_scale_factor(gl_area);
        int scaled_mouse_x = mouse_x * scale_factor;
        int scaled_mouse_y = mouse_y * scale_factor;

        ModifierKey modifiers = ModifierFromState(event_state);

        // TODO: Track click type. Consider having a member that tracks it from GtkGesture.
        Window* app_window = main_window->appWindow();
        app_window->onLeftMouseDrag(scaled_mouse_x, scaled_mouse_y, modifiers,
                                    ClickType::kSingleClick);
    }
}

static void quit_callback(GSimpleAction* action, GVariant* parameter, gpointer app) {
    std::println("quit callback");
    g_application_quit(G_APPLICATION(app));
}

static gboolean key_pressed(GtkEventControllerKey* self,
                            guint keyval,
                            guint keycode,
                            GdkModifierType state,
                            gpointer user_data) {
    // TODO: Why does this occur?
    if (keyval == GDK_KEY_VoidSymbol) return false;

    Window* app_window = static_cast<Window*>(user_data);

    Key key = KeyFromKeyval(keyval);
    ModifierKey modifiers = ModifierFromState(state);
    bool handled = app_window->onKeyDown(key, modifiers);

    if (!handled) {
        guint32 codepoint = gdk_keyval_to_unicode(keyval);
        if (codepoint > 0) {
            char utf8[unicode::kMaxBytesInUTF8Sequence];
            size_t utf8_len = unicode::ToUTF8(codepoint, utf8);

            std::string str8(utf8, utf8_len);
            app_window->onInsertText(str8);

            std::println("str8 = {}, codepoint = {}", str8, codepoint);
        }
    }
    return false;  // TODO: See if we should return true/false here.
}

}  // namespace app

namespace {

constexpr app::Key KeyFromKeyval(guint keyval) {
    // Convert to uppercase since GTK key events are case-sensitive.
    keyval = gdk_keyval_to_upper(keyval);

    constexpr struct {
        guint keyval;
        app::Key key;
    } kKeyMap[] = {
        {GDK_KEY_A, app::Key::kA},
        {GDK_KEY_B, app::Key::kB},
        {GDK_KEY_C, app::Key::kC},
        {GDK_KEY_D, app::Key::kD},
        {GDK_KEY_E, app::Key::kE},
        {GDK_KEY_F, app::Key::kF},
        {GDK_KEY_G, app::Key::kG},
        {GDK_KEY_H, app::Key::kH},
        {GDK_KEY_I, app::Key::kI},
        {GDK_KEY_J, app::Key::kJ},
        {GDK_KEY_K, app::Key::kK},
        {GDK_KEY_L, app::Key::kL},
        {GDK_KEY_M, app::Key::kM},
        {GDK_KEY_N, app::Key::kN},
        {GDK_KEY_O, app::Key::kO},
        {GDK_KEY_P, app::Key::kP},
        {GDK_KEY_Q, app::Key::kQ},
        {GDK_KEY_R, app::Key::kR},
        {GDK_KEY_S, app::Key::kS},
        {GDK_KEY_T, app::Key::kT},
        {GDK_KEY_U, app::Key::kU},
        {GDK_KEY_V, app::Key::kV},
        {GDK_KEY_W, app::Key::kW},
        {GDK_KEY_X, app::Key::kX},
        {GDK_KEY_Y, app::Key::kY},
        {GDK_KEY_Z, app::Key::kZ},
        {GDK_KEY_0, app::Key::k0},
        {GDK_KEY_1, app::Key::k1},
        {GDK_KEY_2, app::Key::k2},
        {GDK_KEY_3, app::Key::k3},
        {GDK_KEY_4, app::Key::k4},
        {GDK_KEY_5, app::Key::k5},
        {GDK_KEY_6, app::Key::k6},
        {GDK_KEY_7, app::Key::k7},
        {GDK_KEY_8, app::Key::k8},
        {GDK_KEY_9, app::Key::k9},
        {GDK_KEY_Return, app::Key::kEnter},
        {GDK_KEY_BackSpace, app::Key::kBackspace},
        {GDK_KEY_Tab, app::Key::kTab},
        {GDK_KEY_Left, app::Key::kLeftArrow},
        {GDK_KEY_Right, app::Key::kRightArrow},
        {GDK_KEY_Down, app::Key::kDownArrow},
        {GDK_KEY_Up, app::Key::kUpArrow},
    };
    for (size_t i = 0; i < std::size(kKeyMap); ++i) {
        if (kKeyMap[i].keyval == keyval) {
            return kKeyMap[i].key;
        }
    }
    return app::Key::kNone;
}

constexpr app::ModifierKey ModifierFromState(guint state) {
    app::ModifierKey modifiers = app::ModifierKey::kNone;
    if (state & GDK_SHIFT_MASK) {
        modifiers |= app::ModifierKey::kShift;
    }
    if (state & GDK_CONTROL_MASK) {
        modifiers |= app::ModifierKey::kControl;
    }
    if (state & GDK_ALT_MASK) {
        modifiers |= app::ModifierKey::kAlt;
    }
    if (state & GDK_SUPER_MASK || state & GDK_META_MASK) {
        modifiers |= app::ModifierKey::kSuper;
    }
    return modifiers;
}

// TODO: Make this work with multiple modifiers.
constexpr const gchar* GtkAccelStringFromModifier(app::ModifierKey modifier) {
    switch (modifier) {
    case app::ModifierKey::kShift:
        return "<Shift>";
    case app::ModifierKey::kControl:
        return "<Control>";
    case app::ModifierKey::kAlt:
        return "<Alt>";
    // TODO: Change this to <Super> after we finish testing. macOS's Command key maps to <Meta>.
    // https://docs.gtk.org/gdk4/flags.ModifierType.html
    case app::ModifierKey::kSuper:
        return "<Meta>";
    default:
        std::println("Error: Could not parse modifier.");
        std::abort();
    }
}

}  // namespace
