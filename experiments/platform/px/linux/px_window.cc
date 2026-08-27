// The GTK3 window: signal wiring on a GtkWindow + GtkDrawingArea pair, and the window half of the
// flat px API.
//
// Every callback does the same three things ST's Windows and macOS entry points do: memset a
// px_event_t, fill the fields its tag needs, and call one funnel, px_linux_send_event. The tag
// values match px.h's enum exactly, so a trace from this backend lines up with the other two event
// for event.
//
// Two structural choices follow directly from the recovered symbol list rather than convention:
//
//   * "size-allocate" on the drawing area drives PX_EVENT_RESIZE (content-area size in application
//     pixels); "configure-event" on the toplevel is a confirmed signal too, but nothing in px.h
//     has a slot for "the window moved" -- there is no such event tag on any of the three
//     platforms -- so it is intentionally left unconnected rather than wired to a no-op.
//
//   * DPI is polled, not subscribed to: ST carries no "notify::scale-factor" string, so
//     gtk_widget_get_scale_factor() is re-read on every draw and compared against the last known
//     value, the same shape as macOS's -windowDidChangeBackingProperties: except driven by a
//     comparison instead of a notification.
//
// One gap, stated rather than papered over: PX_EVENT_CAPTURE_LOST (WM_CAPTURECHANGED's tag) has no
// identified GTK3 equivalent -- GDK's implicit per-widget pointer grab during a button press has
// no "someone stole it" signal the way Win32's SetCapture/ReleaseCapture does -- so this backend
// never emits it.

#include "experiments/platform/px/gl_render_context.h"
#include "experiments/platform/px/linux/px_linux_private.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr double kMinEventFlushInterval = 1.0 / 120.0;

std::vector<px_window_t*>& windows_storage() {
    static std::vector<px_window_t*> windows;
    return windows;
}

std::vector<std::function<void()>>& post_event_callbacks() {
    static std::vector<std::function<void()>> callbacks;
    return callbacks;
}

dummy_px_window_event_handler& dummy_handler() {
    static dummy_px_window_event_handler handler;
    return handler;
}

// Scratch for drag-data-received / drag-drop, valid only for the duration of the dispatch -- the
// same contract px_event_t::paths documents on every backend.
struct PathList {
    std::vector<std::string> storage;
    std::vector<const char*> pointers;

    void assign(gchar** uris) {
        storage.clear();
        pointers.clear();
        if (uris) {
            for (gchar** p = uris; *p; ++p) {
                gchar* path = g_filename_from_uri(*p, nullptr, nullptr);
                storage.emplace_back(path ? path : *p);
                g_free(path);
            }
        }
        pointers.reserve(storage.size());
        for (const std::string& s : storage) {
            pointers.push_back(s.c_str());
        }
    }

    const char* const* data() const { return pointers.empty() ? nullptr : pointers.data(); }
    int count() const { return static_cast<int>(pointers.size()); }
};

PathList& drop_paths() {
    static PathList paths;
    return paths;
}

GdkCursorType gdk_cursor_type_for(px_cursor_t cursor) {
    switch (cursor) {
    case PX_CURSOR_IBEAM:
        return GDK_XTERM;
    case PX_CURSOR_CROSSHAIR:
        return GDK_CROSSHAIR;
    case PX_CURSOR_POINTING_HAND:
        return GDK_HAND2;
    case PX_CURSOR_RESIZE_LEFT_RIGHT:
        return GDK_SB_H_DOUBLE_ARROW;
    case PX_CURSOR_RESIZE_UP_DOWN:
        return GDK_SB_V_DOUBLE_ARROW;
    case PX_CURSOR_ARROW:
    default:
        return GDK_LEFT_PTR;
    }
}

px_input_client* input_client_for(px_window_t* window) {
    return (window && window->handler) ? window->handler->get_input_client() : nullptr;
}

// ── render context ──────────────────────────────────────────────────────────────────────────────

// ── IME ─────────────────────────────────────────────────────────────────────────────────────────

// ST's confirmed GtkIMContext surface is set_client_window/focus_in/focus_out/filter_keypress/
// set_cursor_location/set_use_preedit and, for output, only the "commit" signal -- no
// get_preedit_string, no preedit-changed/-start/-end. That means the Linux backend has no live
// composition display the way macOS's setMarkedText: or Windows' WM_IME_COMPOSITION/GCS_COMPSTR
// do: text becomes visible only once the input method commits it. set_use_preedit(FALSE) still
// suppresses GTK's own preedit popup, matching the Windows WM_IME_SETCONTEXT/
// ISC_SHOWUICOMPOSITIONWINDOW suppression -- both exist because the editor would rather draw
// nothing than let the toolkit draw a preedit window ST/px never learns the contents of.
void on_im_commit(GtkIMContext*, gchar* str, gpointer data) {
    px_window_t* window = static_cast<px_window_t*>(data);
    if (!window || !str) {
        return;
    }
    if (px_input_client* client = input_client_for(window)) {
        client->insert_text(str, px_range_t::none());
        return;
    }
    px_event_t e{};
    e.type = PX_EVENT_CHARACTER;
    px_set_event_text(&e, str, std::strlen(str));
    px_linux_send_event(window, &e);
}

void position_im_cursor(px_window_t* window) {
    px_input_client* client = input_client_for(window);
    if (!client || !window->im_context) {
        return;
    }
    px_range_t actual = px_range_t::none();
    const rect caret = client->first_rect_for_range(client->selected_range(), &actual);
    GdkRectangle area{static_cast<int>(caret.x), static_cast<int>(caret.y),
                      static_cast<int>(caret.w), static_cast<int>(caret.h)};
    gtk_im_context_set_cursor_location(window->im_context, &area);
}

// ── paint ───────────────────────────────────────────────────────────────────────────────────────

gboolean on_draw(GtkWidget* widget, cairo_t* cr, gpointer data) {
    px_window_t* window = static_cast<px_window_t*>(data);
    if (!window || !window->handler) {
        return FALSE;
    }

    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    const int scale_factor = std::max(1, gtk_widget_get_scale_factor(widget));
    const double scale = static_cast<double>(scale_factor);

    // Polled DPI change: see the file comment.
    if (window->did_first_paint && std::abs(scale - window->dpi_scale) > 1e-9) {
        window->dpi_scale = scale;
        px_event_t e{};
        e.type = PX_EVENT_DPI_CHANGED;
        e.size = vec2{static_cast<double>(alloc.width), static_cast<double>(alloc.height)};
        e.dpi_scale_factor = scale;
        px_linux_send_event(window, &e);
    } else {
        window->dpi_scale = scale;
    }

    const vec2 size{static_cast<double>(alloc.width), static_cast<double>(alloc.height)};
    const rect bounds{0.0, 0.0, size.x, size.y};
    const int device_w = static_cast<int>(alloc.width * scale_factor);
    const int device_h = static_cast<int>(alloc.height * scale_factor);

    std::vector<rect> dirty;
    GdkRectangle clip;
    if (gdk_cairo_get_clip_rectangle(cr, &clip)) {
        dirty.push_back(rect{static_cast<double>(clip.x), static_cast<double>(clip.y),
                             static_cast<double>(clip.width), static_cast<double>(clip.height)});
    } else {
        dirty.push_back(bounds);
    }

    if (window->use_gl && px_linux_gl_make_current(window) &&
        px_linux_gl_ensure_target(window, device_w, device_h)) {
        window->handler->pre_paint();
        px_linux_dispatch_post_event_callbacks();

        glBindFramebuffer(GL_FRAMEBUFFER, window->fbo);
        glViewport(0, 0, device_w, device_h);

        if (!window->did_first_paint) {
            dirty.clear();
            dirty.push_back(bounds);
        }
        gl_render_context::normalize_dirty_rects(&dirty, bounds);
        gl_render_context rc(vec2{static_cast<double>(device_w), static_cast<double>(device_h)},
                             scale, dirty.data(), static_cast<int>(dirty.size()));
        window->handler->paint(&rc, rc.paint_bounds(), dirty.data(),
                               static_cast<int>(dirty.size()));
        rc.finish();

        // Single-buffered, like the other two backends: glFlush is the whole presentation step,
        // and gdk_cairo_draw_from_gl is what gets the result from our FBO into GTK3's
        // cairo-composited window -- GTK3 has no GL-native present path the way CAOpenGLLayer or
        // an HDC-backed HGLRC do.
        glFlush();
        gdk_cairo_draw_from_gl(cr, gtk_widget_get_window(widget), window->color_renderbuffer,
                               GL_RENDERBUFFER, scale_factor, 0, 0, device_w, device_h);

        window->did_first_paint = true;
        window->last_flush = px_now();
    } else {
        const fcolor& bg = window->background;
        cairo_set_source_rgba(cr, bg.r, bg.g, bg.b, bg.a);
        cairo_paint_with_alpha(cr, 1.0);
    }
    return FALSE;
}

// ── realize / unrealize ─────────────────────────────────────────────────────────────────────────

void on_realize(GtkWidget* widget, gpointer data) {
    px_window_t* window = static_cast<px_window_t*>(data);
    if (!window) {
        return;
    }
    if (window->use_gl && !px_linux_gl_create(window)) {
        window->use_gl = false;
    }
    if (window->im_context) {
        gtk_im_context_set_client_window(window->im_context, gtk_widget_get_window(widget));
    }
}

void on_unrealize(GtkWidget*, gpointer data) {
    px_linux_gl_destroy(static_cast<px_window_t*>(data));
}

// ── size / focus ────────────────────────────────────────────────────────────────────────────────

void on_size_allocate(GtkWidget*, GtkAllocation* allocation, gpointer data) {
    px_window_t* window = static_cast<px_window_t*>(data);
    if (!window) {
        return;
    }
    const vec2 size{static_cast<double>(allocation->width),
                    static_cast<double>(allocation->height)};
    px_mark_rect_dirty(window, rect{0.0, 0.0, size.x, size.y});

    px_event_t e{};
    e.type = PX_EVENT_RESIZE;
    e.size = size;
    e.dpi_scale_factor = window->dpi_scale;
    px_linux_send_event(window, &e);
}

gboolean on_focus_in_event(GtkWidget*, GdkEventFocus*, gpointer data) {
    px_window_t* window = static_cast<px_window_t*>(data);
    if (!window) {
        return FALSE;
    }
    if (window->im_context) {
        gtk_im_context_focus_in(window->im_context);
    }
    px_event_t e{};
    e.type = PX_EVENT_FOCUS_GAINED;
    px_linux_send_event(window, &e);
    return FALSE;
}

gboolean on_focus_out_event(GtkWidget*, GdkEventFocus*, gpointer data) {
    px_window_t* window = static_cast<px_window_t*>(data);
    if (!window) {
        return FALSE;
    }
    if (window->im_context) {
        gtk_im_context_focus_out(window->im_context);
    }
    px_event_t e{};
    e.type = PX_EVENT_FOCUS_LOST;
    px_linux_send_event(window, &e);
    return FALSE;
}

// ── keyboard ────────────────────────────────────────────────────────────────────────────────────

gboolean on_key_press_event(GtkWidget*, GdkEventKey* event, gpointer data) {
    px_window_t* window = static_cast<px_window_t*>(data);
    if (!window) {
        return FALSE;
    }

    // Idiomatic GTK3 ordering: the input method gets first look, not last. Unlike the concern that
    // motivates macOS's "bindings first" comment (Cmd-S must not type 's'), this is safe here
    // because every real IM context (the plain "simple" engine, ibus, ...) already declines to
    // filter Ctrl/Alt-modified keys -- those are reserved for application shortcuts by convention
    // -- so chord bindings still reach handle_event below unfiltered.
    if (window->im_context && gtk_im_context_filter_keypress(window->im_context, event)) {
        return TRUE;
    }

    const bool is_repeat =
        std::find(window->pressed_keyvals.begin(), window->pressed_keyvals.end(), event->keyval) !=
        window->pressed_keyvals.end();
    if (!is_repeat) {
        window->pressed_keyvals.push_back(event->keyval);
    }

    px_event_t e{};
    e.type = PX_EVENT_KEY;
    e.key = px_linux_keyval_to_px_key(event);
    e.modifiers = px_linux_modifiers(static_cast<GdkModifierType>(event->state));
    e.pressed = true;
    e.repeat = is_repeat;
    const bool consumed = window->handler && window->handler->handle_event(&e);
    px_linux_dispatch_post_event_callbacks();
    return consumed ? TRUE : FALSE;
}

gboolean on_key_release_event(GtkWidget*, GdkEventKey* event, gpointer data) {
    px_window_t* window = static_cast<px_window_t*>(data);
    if (!window) {
        return FALSE;
    }
    window->pressed_keyvals.erase(
        std::remove(window->pressed_keyvals.begin(), window->pressed_keyvals.end(), event->keyval),
        window->pressed_keyvals.end());

    if (window->im_context && gtk_im_context_filter_keypress(window->im_context, event)) {
        return TRUE;
    }

    px_event_t e{};
    e.type = PX_EVENT_KEY;
    e.key = px_linux_keyval_to_px_key(event);
    e.modifiers = px_linux_modifiers(static_cast<GdkModifierType>(event->state));
    e.pressed = false;
    const bool consumed = window->handler && window->handler->handle_event(&e);
    px_linux_dispatch_post_event_callbacks();
    return consumed ? TRUE : FALSE;
}

// ── mouse ───────────────────────────────────────────────────────────────────────────────────────

px_mouse_button button_from_gdk(guint button) {
    switch (button) {
    case 1:
        return PX_MOUSE_LEFT;
    case 2:
        return PX_MOUSE_MIDDLE;
    case 3:
        return PX_MOUSE_RIGHT;
    case 8:
        return PX_MOUSE_X1;
    case 9:
        return PX_MOUSE_X2;
    default:
        return PX_MOUSE_NONE;
    }
}

gboolean on_button_press_event(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    px_window_t* window = static_cast<px_window_t*>(data);
    if (!window) {
        return FALSE;
    }
    // GDK_2BUTTON_PRESS/GDK_3BUTTON_PRESS fire in addition to a plain GDK_BUTTON_PRESS for the
    // same physical click, not instead of it (unlike Win32's mutually exclusive
    // WM_LBUTTONDOWN/DBLCLK) -- counting those too would double-report every multi-click. Click
    // count is tracked by hand below against gtk-double-click-time/-distance instead.
    if (event->type != GDK_BUTTON_PRESS) {
        return FALSE;
    }
    const px_mouse_button button = button_from_gdk(event->button);
    if (button == PX_MOUSE_NONE) {
        return FALSE;
    }

    gtk_widget_grab_focus(widget);

    gint double_click_time = 400;
    gint double_click_distance = 5;
    if (GtkSettings* settings = gtk_widget_get_settings(widget)) {
        g_object_get(settings, "gtk-double-click-time", &double_click_time,
                     "gtk-double-click-distance", &double_click_distance, nullptr);
    }
    const vec2 pos{event->x, event->y};
    const guint32 elapsed = event->time - window->last_click_time;
    const bool within_time =
        window->last_click_time != 0 && elapsed <= static_cast<guint32>(double_click_time);
    const bool within_distance =
        std::abs(pos.x - window->last_click_pos.x) <= double_click_distance &&
        std::abs(pos.y - window->last_click_pos.y) <= double_click_distance;
    window->click_count = (button == window->last_click_button && within_time && within_distance)
                              ? window->click_count + 1
                              : 1;
    window->last_click_time = event->time;
    window->last_click_button = button;
    window->last_click_pos = pos;

    px_event_t e{};
    e.type = PX_EVENT_MOUSE_BUTTON;
    e.pos = pos;
    e.button = button;
    e.pressed = true;
    e.click_count = window->click_count;
    e.modifiers = px_linux_modifiers(static_cast<GdkModifierType>(event->state));
    px_linux_send_event(window, &e);
    return TRUE;
}

gboolean on_button_release_event(GtkWidget*, GdkEventButton* event, gpointer data) {
    px_window_t* window = static_cast<px_window_t*>(data);
    if (!window) {
        return FALSE;
    }
    const px_mouse_button button = button_from_gdk(event->button);
    if (button == PX_MOUSE_NONE) {
        return FALSE;
    }
    px_event_t e{};
    e.type = PX_EVENT_MOUSE_BUTTON;
    e.pos = vec2{event->x, event->y};
    e.button = button;
    e.pressed = false;
    e.click_count = 1;
    e.modifiers = px_linux_modifiers(static_cast<GdkModifierType>(event->state));
    px_linux_send_event(window, &e);
    return TRUE;
}

gboolean on_motion_notify_event(GtkWidget*, GdkEventMotion* event, gpointer data) {
    px_window_t* window = static_cast<px_window_t*>(data);
    if (!window) {
        return FALSE;
    }
    px_event_t e{};
    e.type = PX_EVENT_MOUSE_MOTION;
    e.pos = vec2{event->x, event->y};
    e.modifiers = px_linux_modifiers(static_cast<GdkModifierType>(event->state));
    px_linux_send_event(window, &e);
    return FALSE;
}

gboolean on_leave_notify_event(GtkWidget*, GdkEventCrossing* event, gpointer data) {
    px_window_t* window = static_cast<px_window_t*>(data);
    if (!window) {
        return FALSE;
    }
    px_event_t e{};
    e.type = PX_EVENT_MOUSE_LEAVE;
    e.pos = vec2{event->x, event->y};
    e.modifiers = px_linux_modifiers(static_cast<GdkModifierType>(event->state));
    px_linux_send_event(window, &e);
    return FALSE;
}

gboolean on_scroll_event(GtkWidget*, GdkEventScroll* event, gpointer data) {
    px_window_t* window = static_cast<px_window_t*>(data);
    if (!window) {
        return FALSE;
    }
    px_event_t e{};
    e.type = PX_EVENT_SCROLL;
    e.pos = vec2{event->x, event->y};
    e.modifiers = px_linux_modifiers(static_cast<GdkModifierType>(event->state));

    if (event->direction == GDK_SCROLL_SMOOTH) {
        e.precise_scroll = true;
        // GdkEventScroll::delta_x/delta_y are plain struct fields (no accessor needed, which is
        // why neither appears in the resolved symbol list); gdk_event_is_scroll_stop_event is the
        // one confirmed call, used to detect a trackpad fling's terminal zero-delta event rather
        // than to read the deltas themselves. GDK's sign convention is "positive moves content
        // right/down", the opposite of px.h's "positive dy scrolls content up", so both axes are
        // negated.
        e.scroll_delta = vec2{-event->delta_x * 16.0, -event->delta_y * 16.0};
    } else {
        e.precise_scroll = false;
        constexpr double kStep = 16.0 * 3.0;
        switch (event->direction) {
        case GDK_SCROLL_UP:
            e.scroll_delta = vec2{0.0, kStep};
            break;
        case GDK_SCROLL_DOWN:
            e.scroll_delta = vec2{0.0, -kStep};
            break;
        case GDK_SCROLL_LEFT:
            e.scroll_delta = vec2{kStep, 0.0};
            break;
        case GDK_SCROLL_RIGHT:
            e.scroll_delta = vec2{-kStep, 0.0};
            break;
        default:
            break;
        }
    }
    px_linux_send_event(window, &e);
    return TRUE;
}

// ── drag and drop ───────────────────────────────────────────────────────────────────────────────

gboolean on_drag_motion(
    GtkWidget*, GdkDragContext* context, gint x, gint y, guint time, gpointer data) {
    px_window_t* window = static_cast<px_window_t*>(data);
    if (!window || !window->handler) {
        return FALSE;
    }
    const vec2 pos{static_cast<double>(x), static_cast<double>(y)};
    const bool accept =
        window->drag_active
            ? window->handler->drag_drop_motion(pos, drop_paths().data(), drop_paths().count())
            : window->handler->drag_drop_enter(pos, drop_paths().data(), drop_paths().count());
    window->drag_active = true;
    gdk_drag_status(context, accept ? GDK_ACTION_COPY : static_cast<GdkDragAction>(0), time);
    return TRUE;
}

void on_drag_leave(GtkWidget*, GdkDragContext*, guint, gpointer data) {
    px_window_t* window = static_cast<px_window_t*>(data);
    if (!window) {
        return;
    }
    if (window->drag_active && window->handler) {
        window->handler->drag_drop_exit();
    }
    window->drag_active = false;
}

gboolean on_drag_drop(
    GtkWidget* widget, GdkDragContext* context, gint, gint, guint time, gpointer) {
    GdkAtom target = gtk_drag_dest_find_target(widget, context, nullptr);
    if (target == GDK_NONE) {
        return FALSE;
    }
    gtk_drag_get_data(widget, context, target, time);
    return TRUE;
}

void on_drag_data_received(GtkWidget*,
                           GdkDragContext* context,
                           gint x,
                           gint y,
                           GtkSelectionData* selection_data,
                           guint,
                           guint time,
                           gpointer data) {
    px_window_t* window = static_cast<px_window_t*>(data);
    bool accepted = false;
    if (window && window->handler) {
        gchar** uris = gtk_selection_data_get_uris(selection_data);
        drop_paths().assign(uris);
        g_strfreev(uris);

        const vec2 pos{static_cast<double>(x), static_cast<double>(y)};
        accepted =
            window->handler->drag_drop_accept(pos, drop_paths().data(), drop_paths().count());
        if (accepted) {
            px_event_t e{};
            e.type = PX_EVENT_DROP_FILES;
            e.pos = pos;
            e.paths = drop_paths().data();
            e.path_count = drop_paths().count();
            px_linux_send_event(window, &e);
        }
    }
    gtk_drag_finish(context, accepted, FALSE, time);
    if (window) {
        window->drag_active = false;
    }
}

// ── close / destroy ─────────────────────────────────────────────────────────────────────────────

gboolean on_delete_event(GtkWidget*, GdkEvent*, gpointer data) {
    px_window_t* window = static_cast<px_window_t*>(data);
    if (!window || !window->handler || window->handler->can_close_without_prompt()) {
        return FALSE;  // let the default handler destroy the window
    }
    px_window_t* target = window;
    window->handler->try_close([target](bool should_close) {
        if (should_close) {
            px_destroy_window(target);
        }
    });
    return TRUE;  // block the default handler; try_close's callback decides
}

void on_destroy(GtkWidget*, gpointer data) {
    px_window_t* window = static_cast<px_window_t*>(data);
    if (!window || window->closing) {
        return;
    }
    window->closing = true;

    px_event_t e{};
    e.type = PX_EVENT_DESTROY;
    px_linux_send_event(window, &e);

    // Equivalent of -[PXApplicationDelegate applicationShouldTerminateAfterLastWindowClosed:]
    // returning YES, and of the Windows backend's PostQuitMessage when windows_storage() empties.
    const bool any_left = std::any_of(windows_storage().begin(), windows_storage().end(),
                                      [](const px_window_t* w) { return !w->closing; });
    if (!any_left) {
        px_exit_event_loop();
    }
}

// ── animation
// ────────────────────────────────────────────────────────────────────────────────────

// gtk_widget_add_tick_callback is confirmed in ST's resolved symbol list -- its frame clock is the
// vsync-paced source, the same conceptual role a CVDisplayLink plays on macOS.
gboolean on_tick(GtkWidget*, GdkFrameClock*, gpointer data) {
    px_window_t* window = static_cast<px_window_t*>(data);
    if (window && window->handler && !window->closing) {
        window->handler->animation_tick(px_now());
        px_linux_flush_dirty_rects(window);
    }
    return G_SOURCE_CONTINUE;
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// THE FUNNEL
// ─────────────────────────────────────────────────────────────────────────────────────────────────

void px_linux_send_event(px_window_t* window, px_event_t* event) {
    if (!window || !window->handler) {
        return;
    }
    event->window = window;
    window->handler->handle_event(event);

    // Key (0) and character (1) events skip the repaint flush; anything from mouse button (2) up
    // can trigger one, rate-limited -- the same tail every backend's send_event has.
    if (window->did_first_paint && event->type >= PX_EVENT_MOUSE_BUTTON) {
        const double now = px_now();
        if (now - window->last_flush > kMinEventFlushInterval) {
            window->handler->pre_paint();
            px_linux_flush_dirty_rects(window);
        }
    }

    px_linux_dispatch_post_event_callbacks();
}

void px_linux_flush_dirty_rects(px_window_t* window) {
    if (!window || window->dirty.empty() || !window->area) {
        return;
    }
    // GTK widget coordinates are already application pixels (== px.h's points at any scale
    // factor), unlike Win32's physical-pixel client area, so no scale multiply is needed here.
    for (const rect& r : window->dirty) {
        gtk_widget_queue_draw_area(
            window->area, static_cast<int>(std::floor(r.x)), static_cast<int>(std::floor(r.y)),
            static_cast<int>(std::ceil(r.w)), static_cast<int>(std::ceil(r.h)));
    }
    window->dirty.clear();
}

void px_linux_dispatch_post_event_callbacks() {
    if (post_event_callbacks().empty()) {
        return;
    }
    std::vector<std::function<void()>> pending;
    pending.swap(post_event_callbacks());
    for (const std::function<void()>& fn : pending) {
        fn();
    }
}

const std::vector<px_window_t*>& px_linux_all_windows() { return windows_storage(); }

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// FLAT API: WINDOWS
// ─────────────────────────────────────────────────────────────────────────────────────────────────

px_window_t* px_create_window(px_window_event_handler* handler,
                              px_window_t* parent,
                              double width,
                              double height,
                              const char* title,
                              fcolor background,
                              uint32_t flags) {
    px_window_t* window = new px_window_t();
    window->handler = handler ? handler : &dummy_handler();
    window->background = background;
    window->use_gl = g_getenv("PX_NO_GL") == nullptr;

    window->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window->window), title ? title : "");
    gtk_window_set_default_size(GTK_WINDOW(window->window), static_cast<int>(width),
                                static_cast<int>(height));
    gtk_window_set_decorated(GTK_WINDOW(window->window), (flags & PX_WINDOW_TITLED) != 0);
    gtk_window_set_resizable(GTK_WINDOW(window->window), (flags & PX_WINDOW_RESIZABLE) != 0);
    if (parent && parent->window) {
        gtk_window_set_transient_for(GTK_WINDOW(window->window), GTK_WINDOW(parent->window));
    }

    window->area = gtk_drawing_area_new();
    // ST configures the drawing area for direct/manual GL rendering rather than delegating to
    // GtkGLArea (confirmed absent -- no gtk_gl_area_* symbol anywhere): app_paintable so GTK does
    // not clear the widget's background itself, double_buffered off so GTK's own cairo backing
    // store does not fight the FBO blit in on_draw, and an RGBA visual so a translucent
    // px_event_t::background alpha actually composites instead of being opaque-clamped.
    gtk_widget_set_app_paintable(window->area, TRUE);
    // gtk_widget_set_double_buffered is deprecated as of GTK 3.14 (in favor of always-on
    // double-buffering via GL surfaces), but it is the confirmed symbol ST resolves, so the
    // warning is suppressed the same way mac/px_gl_layer.mm suppresses CAOpenGLLayer/CGL
    // deprecation.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    gtk_widget_set_double_buffered(window->area, FALSE);
#pragma clang diagnostic pop
    if (GdkScreen* screen = gtk_widget_get_screen(window->area)) {
        if (GdkVisual* rgba_visual = gdk_screen_get_rgba_visual(screen)) {
            gtk_widget_set_visual(window->area, rgba_visual);
        }
    }
    gtk_widget_set_can_focus(window->area, TRUE);
    gtk_widget_add_events(window->area, GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
                                            GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                                            GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK |
                                            GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK |
                                            GDK_FOCUS_CHANGE_MASK | GDK_STRUCTURE_MASK);
    gtk_container_add(GTK_CONTAINER(window->window), window->area);

    window->im_context = gtk_im_multicontext_new();
    gtk_im_context_set_use_preedit(window->im_context, FALSE);
    g_signal_connect(window->im_context, "commit", G_CALLBACK(on_im_commit), window);

    g_signal_connect(window->area, "realize", G_CALLBACK(on_realize), window);
    g_signal_connect(window->area, "unrealize", G_CALLBACK(on_unrealize), window);
    g_signal_connect(window->area, "draw", G_CALLBACK(on_draw), window);
    g_signal_connect(window->area, "size-allocate", G_CALLBACK(on_size_allocate), window);
    g_signal_connect(window->area, "focus-in-event", G_CALLBACK(on_focus_in_event), window);
    g_signal_connect(window->area, "focus-out-event", G_CALLBACK(on_focus_out_event), window);
    g_signal_connect(window->area, "key-press-event", G_CALLBACK(on_key_press_event), window);
    g_signal_connect(window->area, "key-release-event", G_CALLBACK(on_key_release_event), window);
    g_signal_connect(window->area, "button-press-event", G_CALLBACK(on_button_press_event),
                     window);
    g_signal_connect(window->area, "button-release-event", G_CALLBACK(on_button_release_event),
                     window);
    g_signal_connect(window->area, "motion-notify-event", G_CALLBACK(on_motion_notify_event),
                     window);
    g_signal_connect(window->area, "leave-notify-event", G_CALLBACK(on_leave_notify_event),
                     window);
    g_signal_connect(window->area, "scroll-event", G_CALLBACK(on_scroll_event), window);
    gtk_widget_add_tick_callback(window->area, on_tick, window, nullptr);

    // Manual target list rather than gtk_drag_dest_add_text_targets: only "text/uri-list" is
    // wanted (file drops), which is the one URI-specific target ST's confirmed symbol set does not
    // have a one-line convenience call for -- gtk_target_list_new/_add/_table_new_from_list are.
    GtkTargetList* targets = gtk_target_list_new(nullptr, 0);
    gtk_target_list_add(targets, gdk_atom_intern("text/uri-list", FALSE), 0, 0);
    gint n_targets = 0;
    GtkTargetEntry* table = gtk_target_table_new_from_list(targets, &n_targets);
    gtk_drag_dest_set(window->area, static_cast<GtkDestDefaults>(0), table, n_targets,
                      GDK_ACTION_COPY);
    gtk_target_table_free(table, n_targets);
    gtk_target_list_unref(targets);
    g_signal_connect(window->area, "drag-motion", G_CALLBACK(on_drag_motion), window);
    g_signal_connect(window->area, "drag-leave", G_CALLBACK(on_drag_leave), window);
    g_signal_connect(window->area, "drag-drop", G_CALLBACK(on_drag_drop), window);
    g_signal_connect(window->area, "drag-data-received", G_CALLBACK(on_drag_data_received),
                     window);

    g_signal_connect(window->window, "delete-event", G_CALLBACK(on_delete_event), window);
    g_signal_connect(window->window, "destroy", G_CALLBACK(on_destroy), window);

    gtk_widget_show_all(window->window);
    window->dpi_scale = gtk_widget_get_scale_factor(window->area);

    windows_storage().push_back(window);
    return window;
}

void px_destroy_window(px_window_t* window) {
    if (!window) {
        return;
    }
    if (window->cursor_handle) {
        g_object_unref(window->cursor_handle);
        window->cursor_handle = nullptr;
    }
    if (window->im_context) {
        g_object_unref(window->im_context);
        window->im_context = nullptr;
    }
    if (window->window) {
        gtk_widget_destroy(
            window->window);  // triggers on_destroy for the PX_EVENT_DESTROY dispatch
        window->window = nullptr;
    }

    std::vector<px_window_t*>& all = windows_storage();
    all.erase(std::remove(all.begin(), all.end(), window), all.end());
    delete window;
}

void px_show_window(px_window_t* window) {
    if (window && window->window) {
        gtk_widget_show(window->window);
        gtk_window_present(GTK_WINDOW(window->window));
    }
}

void px_show_window_without_focus(px_window_t* window) {
    if (window && window->window) {
        gtk_widget_show(window->window);
    }
}

void px_hide_window(px_window_t* window) {
    if (window && window->window) {
        gtk_widget_hide(window->window);
    }
}

void px_close_window(px_window_t* window) {
    if (window && window->window) {
        // Runs the same delete-event -> can_close_without_prompt()/try_close() path a user
        // clicking the window's own close button would, the asynchronous analogue of
        // PostMessageW(WM_CLOSE).
        gtk_window_close(GTK_WINDOW(window->window));
    }
}

void px_set_window_title(px_window_t* window, const char* title) {
    if (window && window->window) {
        gtk_window_set_title(GTK_WINDOW(window->window), title ? title : "");
    }
}

vec2 px_window_size(px_window_t* window) {
    if (!window || !window->area) {
        return vec2{};
    }
    GtkAllocation alloc;
    gtk_widget_get_allocation(window->area, &alloc);
    return vec2{static_cast<double>(alloc.width), static_cast<double>(alloc.height)};
}

vec2 px_window_position(px_window_t* window) {
    if (!window || !window->window) {
        return vec2{};
    }
    gint x = 0, y = 0;
    gtk_window_get_position(GTK_WINDOW(window->window), &x, &y);
    return vec2{static_cast<double>(x), static_cast<double>(y)};
}

void px_set_window_size(px_window_t* window, double width, double height) {
    if (window && window->window) {
        gtk_window_resize(GTK_WINDOW(window->window), static_cast<int>(width),
                          static_cast<int>(height));
    }
}

void px_set_window_position(px_window_t* window, vec2 position) {
    if (window && window->window) {
        gtk_window_move(GTK_WINDOW(window->window), static_cast<int>(position.x),
                        static_cast<int>(position.y));
    }
}

double px_window_dpi_scale_factor(px_window_t* window) { return window ? window->dpi_scale : 1.0; }

void px_set_full_screen(px_window_t* window, bool full_screen) {
    if (!window || !window->window) {
        return;
    }
    window->want_full_screen = full_screen;
    if (full_screen) {
        gtk_window_fullscreen(GTK_WINDOW(window->window));
    } else {
        gtk_window_unfullscreen(GTK_WINDOW(window->window));
    }
}

void px_mark_rect_dirty(px_window_t* window, rect r) {
    if (window && !r.empty()) {
        window->dirty.push_back(r);
    }
}

void px_mark_dirty(px_window_t* window) {
    if (!window) {
        return;
    }
    const vec2 size = px_window_size(window);
    px_mark_rect_dirty(window, rect{0.0, 0.0, size.x, size.y});
}

void px_set_cursor(px_window_t* window, px_cursor_t cursor) {
    if (!window || !window->area) {
        return;
    }
    window->cursor = cursor;
    GdkWindow* gdk_win = gtk_widget_get_window(window->area);
    if (!gdk_win) {
        return;
    }
    if (window->cursor_handle) {
        g_object_unref(window->cursor_handle);
    }
    window->cursor_handle =
        gdk_cursor_new_for_display(gdk_window_get_display(gdk_win), gdk_cursor_type_for(cursor));
    gdk_window_set_cursor(gdk_win, window->cursor_handle);
}

void px_reset_cursor(px_window_t* window) {
    if (!window || !window->area || !window->handler) {
        return;
    }
    GdkWindow* gdk_win = gtk_widget_get_window(window->area);
    if (!gdk_win) {
        return;
    }
    GdkSeat* seat = gdk_display_get_default_seat(gdk_window_get_display(gdk_win));
    gint x = 0, y = 0;
    gdk_window_get_device_position(gdk_win, gdk_seat_get_pointer(seat), &x, &y, nullptr);
    px_set_cursor(window, window->handler->calculate_cursor(
                              vec2{static_cast<double>(x), static_cast<double>(y)}));
}

void px_callback_after_event(std::function<void()> fn) {
    post_event_callbacks().push_back(std::move(fn));
}
