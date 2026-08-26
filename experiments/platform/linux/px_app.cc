// Process-level half of the GTK3 backend: init, the main loop, timers, and the odds and ends of the
// flat API that are not per-window.
//
// ST calls the ordinary blocking gtk_main() -- gtk_main, gtk_main_level and gtk_main_quit are all
// in the resolved symbol list, and nothing suggests a hand-rolled GMainLoop replacement. That is at
// odds with pre_sleep() needing a hook at the moment the loop is about to block, until the rest of
// the import table is read: ST also links g_source_new, g_source_attach and g_source_set_name
// directly (not through the dlsym shim -- these are ordinary glib imports), which is the
// ingredients list for a hand-written GSource. A GSource's prepare() vfunc runs on every iteration
// right before GLib would poll(), which is precisely the slot Windows uses PeekMessageW's queue-
// empty return for and macOS gets from NSApp's run loop observers. That GSource is reproduced here
// instead of the loop shape itself.

#include "experiments/platform/linux/px_linux_private.h"

#include <chrono>

namespace {

px_application_event_handler* g_app_handler = nullptr;
px_application_event_handler g_default_app_handler;
std::chrono::steady_clock::time_point g_start;

px_application_event_handler& app_handler() {
  return g_app_handler ? *g_app_handler : g_default_app_handler;
}

// ── the pre_sleep GSource ───────────────────────────────────────────────────────────────────────

gboolean pre_sleep_prepare(GSource*, gint* timeout) {
  // Iterated over a copy: a handler is allowed to destroy a window from pre_sleep().
  const std::vector<px_window_t*> snapshot = px_linux_all_windows();
  for (px_window_t* window : snapshot) {
    if (window->handler && !window->closing) {
      window->handler->pre_sleep();
    }
    px_linux_flush_dirty_rects(window);
  }
  // Never itself ready to dispatch; only its side effect (the flush above) matters. -1 imposes no
  // timeout of its own, deferring to whatever other source has the nearest deadline.
  *timeout = -1;
  return FALSE;
}

gboolean pre_sleep_check(GSource*) {
  return FALSE;
}

gboolean pre_sleep_dispatch(GSource*, GSourceFunc, gpointer) {
  return G_SOURCE_CONTINUE;
}

GSourceFuncs kPreSleepFuncs = {
    pre_sleep_prepare,
    pre_sleep_check,
    pre_sleep_dispatch,
    nullptr,
};

void install_pre_sleep_source() {
  GSource* source = g_source_new(&kPreSleepFuncs, sizeof(GSource));
  g_source_set_name(source, "px pre_sleep");
  g_source_set_priority(source, G_PRIORITY_LOW);
  g_source_attach(source, nullptr);
  // Intentionally never unref'd/destroyed: it lives for the process, like the windows_storage()
  // vector it walks.
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// FLAT API: PROCESS
// ─────────────────────────────────────────────────────────────────────────────────────────────────

void px_init(const char* app_name, const char* bundle_id, int argc, char** argv, uint32_t flags) {
  (void)app_name;
  (void)bundle_id;
  (void)argc;
  (void)argv;
  (void)flags;

  g_start = std::chrono::steady_clock::now();
  gtk_init(nullptr, nullptr);
  install_pre_sleep_source();
}

void px_set_application_event_handler(px_application_event_handler* handler) {
  g_app_handler = handler;
}

void px_run_event_loop() {
  gtk_main();
}

void px_exit_event_loop() {
  if (gtk_main_level() > 0) {
    gtk_main_quit();
  }
}

void px_set_timeout(std::function<void()> fn, int milliseconds) {
  auto* closure = new std::function<void()>(std::move(fn));
  g_timeout_add(
      static_cast<guint>(milliseconds),
      [](gpointer data) -> gboolean {
        auto* fn = static_cast<std::function<void()>*>(data);
        (*fn)();
        delete fn;
        return G_SOURCE_REMOVE;
      },
      closure);
}

bool px_os_in_dark_mode() {
  GtkSettings* settings = gtk_settings_get_default();
  if (!settings) {
    return false;
  }
  gboolean prefer_dark = FALSE;
  g_object_get(settings, "gtk-application-prefer-dark-theme", &prefer_dark, nullptr);
  if (prefer_dark) {
    return true;
  }
  // Cross-checked against the theme name, the way ST watches notify::gtk-theme-name: a theme named
  // e.g. "Adwaita-dark" can be dark without the application-level preference being set.
  gchar* theme_name = nullptr;
  g_object_get(settings, "gtk-theme-name", &theme_name, nullptr);
  const bool name_says_dark = theme_name && g_strstr_len(theme_name, -1, "dark") != nullptr;
  g_free(theme_name);
  return name_says_dark;
}

double px_caret_blink_time() {
  GtkSettings* settings = gtk_settings_get_default();
  if (!settings) {
    return 0.5;
  }
  gboolean blink = FALSE;
  gint blink_ms = 1200;
  g_object_get(settings, "gtk-cursor-blink", &blink, "gtk-cursor-blink-time", &blink_ms, nullptr);
  if (!blink || blink_ms <= 0) {
    return 0.0;  // "do not blink"
  }
  // gtk-cursor-blink-time is a full on+off cycle, matching the "seconds per phase" contract
  // px_caret_blink_time documents the same way ST's macOS backend averages NSTextInsertionPoint's
  // two phase durations.
  return blink_ms / 2000.0;
}

void px_show_error(px_window_t* parent, const char* message) {
  GtkWidget* dialog =
      gtk_message_dialog_new(parent ? GTK_WINDOW(parent->window) : nullptr,
                             GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s",
                             message ? message : "");
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

void px_open_url(const char* url) {
  if (!url) {
    return;
  }
  // gtk_show_uri is the confirmed symbol ST resolves -- not gtk_show_uri_on_window, its GTK 3.22+
  // replacement, which needs a parent GtkWindow this API does not have on hand. Newer GTK headers
  // deprecate it; suppressed for the same reason mac/px_gl_layer.mm suppresses CAOpenGLLayer/CGL
  // deprecation warnings -- ST 4200 ships on the older call, and mirroring that is the point.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  GError* error = nullptr;
  if (!gtk_show_uri(gdk_screen_get_default(), url, GDK_CURRENT_TIME, &error)) {
    if (error) {
      g_error_free(error);
    }
  }
#pragma clang diagnostic pop
}

double px_now() {
  const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - g_start;
  return elapsed.count();
}
