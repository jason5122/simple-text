// Shared internals of the GTK3 backend. Portable code sees px_window_t as an opaque forward
// declaration, exactly as it does on macOS and Windows.
//
// ST's own binary does not link GTK -- gtk_init, gtk_window_new and the rest of the ~280 gtk_/gdk_
// entry points it uses are resolved with dlopen+dlsym against whichever of GTK2/GTK3 the runtime
// has (the string table carries the bare symbol names; the NEEDED list has no libgtk, libgdk or
// libcairo). That indirection is a distribution trick -- one binary running against either toolkit
// version -- not part of the platform abstraction, so it is not reproduced here: this backend
// links gtk+-3.0 normally through pkg-config, as instructed. The GTK3 call sequence itself mirrors
// what the recovered symbol list says ST actually calls.

#pragma once

#include <gtk/gtk.h>

#include <vector>

#include "experiments/platform/px/px.h"
#include "experiments/platform/px/px_gl.h"

struct px_window_t {
    GtkWidget* window = nullptr;  // GtkWindow, the toplevel.
    GtkWidget* area = nullptr;    // GtkDrawingArea, the whole content area.
    GdkGLContext* gl_context = nullptr;
    GtkIMContext* im_context = nullptr;

    px_window_event_handler* handler = nullptr;

    bool did_first_paint = false;
    bool closing = false;
    bool use_gl = true;

    // Device pixels per point. gtk_widget_get_scale_factor() is polled rather than subscribed to
    // -- ST carries no "notify::scale-factor" string, so this is re-read on size-allocate/draw and
    // compared against the last known value to synthesize PX_EVENT_DPI_CHANGED.
    double dpi_scale = 1.0;

    fcolor background{0.0f, 0.0f, 0.0f, 1.0f};
    px_cursor_t cursor = PX_CURSOR_ARROW;
    GdkCursor* cursor_handle = nullptr;

    // Pending regions in window-space points, drained into gtk_widget_queue_draw_area.
    std::vector<rect> dirty;

    double last_flush = 0.0;

    // GL render target: GTK3's compositor is cairo, not GL, so unlike the CAOpenGLLayer/HGLRC
    // backings on macOS and Windows there is no framebuffer handed to us. This one is owned, sized
    // to the drawing area's allocation in device pixels, and blitted into the "draw" signal's
    // cairo_t with gdk_cairo_draw_from_gl every frame. Recreated on resize.
    GLuint fbo = 0;
    GLuint color_renderbuffer = 0;
    GLuint stencil_renderbuffer = 0;
    int fbo_width = 0;
    int fbo_height = 0;

    // Currently-held keyvals, so key-press-event can report repeat. GDK's detectable-autorepeat
    // mode (the default) sends only repeated press events with no interleaved release, unlike
    // NSEvent's isARepeat or WM_KEYDOWN's lParam bit 30 -- there is no field to read it from
    // directly.
    std::vector<guint> pressed_keyvals;

    // Manual click-count tracking against gtk-double-click-time/gtk-double-click-distance. Needed
    // because GDK delivers a double click as *two* button-press-event signals -- a plain
    // GDK_BUTTON_PRESS immediately followed by a GDK_2BUTTON_PRESS -- rather than Win32's mutually
    // exclusive WM_LBUTTONDOWN/WM_LBUTTONDBLCLK, so counting has to be done by hand from timing
    // and position the way ST's confirmed use of those two settings implies.
    guint32 last_click_time = 0;
    int last_click_button = 0;
    vec2 last_click_pos;
    int click_count = 0;

    // Set between drag-motion's first call and drag-leave/drag-drop, so drag_drop_enter fires once
    // per hover rather than on every motion event the way drag-motion itself repeats.
    bool drag_active = false;

    // Saved across a fullscreen toggle. GTK has no synchronous "are we fullscreen now" query --
    // window-state-event is asynchronous -- so the requested state is tracked here instead.
    bool want_full_screen = false;
};

// px_window.cc
void px_linux_send_event(px_window_t* window, px_event_t* event);
void px_linux_flush_dirty_rects(px_window_t* window);
void px_linux_dispatch_post_event_callbacks();
const std::vector<px_window_t*>& px_linux_all_windows();

// px_gl.cc
bool px_linux_gl_create(px_window_t* window);
void px_linux_gl_destroy(px_window_t* window);
bool px_linux_gl_make_current(px_window_t* window);
// (Re)targets the FBO to the given device-pixel size if it does not already match. Returns false
// if no usable framebuffer exists (allocation not yet realized, or the context never came up).
bool px_linux_gl_ensure_target(px_window_t* window, int width, int height);

// px_keycode.cc
uint32_t px_linux_modifiers(GdkModifierType state);
px_key px_linux_keyval_to_px_key(GdkEventKey* event);
