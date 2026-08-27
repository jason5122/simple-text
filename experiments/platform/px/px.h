// Public interface for the "px" platform layer.
//
// This is a clean-room reimplementation of the platform abstraction used by Sublime Text, whose
// design was recovered from its shipping binaries (macOS arm64 slice retains full local symbols;
// the Windows build retains MSVC RTTI). The shape being mirrored:
//
//   * App -> OS is a flat C API over opaque handles (px_window_t*, px_font_t*). There is no
//     `Window` base class to subclass. ST ships ~151 such functions.
//
//   * OS -> app is exactly one interface, `px_window_event_handler`, with 13 virtuals + dtor.
//     Its methods are per-*category* (paint, drag-drop, close, cursor, IME) and never
//     per-message: there is no `on_key_down`.
//
//   * All input arrives through a single slot, `handle_event`, carrying one memset POD tagged
//     union. ST's is 0x158 bytes with `int type` at +0 and `px_window_t*` at +8. The type tags are
//     shared across platforms: WM_SIZE and -[PXWindowDelegate windowDidResize:] both produce 8.
//
//   * std::function-style callbacks are used only for async completions (dialogs, timers,
//     try_close), never on the input path.
//
// The px_event_type values below are the ones confirmed in the binary, so traces from the two
// implementations line up. Gaps are tags ST uses that were not identified.

#pragma once

#include <cstdint>
#include <functional>

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// OPAQUE HANDLES
// ─────────────────────────────────────────────────────────────────────────────────────────────────
// Defined only inside the per-platform translation units. Portable code never sees the contents.
struct px_window_t;

class px_render_context;
class px_window_event_handler;
class px_application_event_handler;
class px_input_client;

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// GEOMETRY
// ─────────────────────────────────────────────────────────────────────────────────────────────────
// Coordinates are in device-independent points with a top-left origin and y growing downward. The
// Cocoa backend flips them; -[PXView isFlipped] returns YES in ST for the same reason.
struct vec2 {
    double x = 0.0;
    double y = 0.0;
};

// 4 doubles, matching ST: flush_dirty_rects walks its dirty vector with a 0x20 stride and loads
// [x22], [x22+0x10] as two pairs.
struct rect {
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;

    constexpr double right() const { return x + w; }
    constexpr double bottom() const { return y + h; }
    constexpr bool empty() const { return w <= 0.0 || h <= 0.0; }
};

// Device-pixel rectangle, stored as edges. This is the representation used by ST's render
// contexts: gl_render_context::apply_clip loads four packed ints and computes right-left and
// bottom-top immediately before glScissor.
struct recti {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;

    constexpr int width() const { return right - left; }
    constexpr int height() const { return bottom - top; }
    constexpr bool empty() const { return width() <= 0 || height() <= 0; }
};

struct fcolor {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
};

// ST's fill_mode also represents textured/repeating fills. Solid color is the first useful slice
// of that type and is the only one implemented until px_texture is reconstructed.
struct fill_mode {
    fcolor color;

    fill_mode() = default;
    explicit fill_mode(fcolor value) : color(value) {}
};

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// KEYS
// ─────────────────────────────────────────────────────────────────────────────────────────────────
// A px_key is either a Unicode codepoint, or PX_KEY_NAMED | code for keys that have no character.
// ST uses exactly this encoding, with Win32 virtual-key numbering as the canonical code space --
// the Mac backend translates *to* VK values rather than the other way around. Observed in the
// binary: 0x8000000D is Return, and 0x8000010F is keypad Enter, chosen when lParam bit 24 (the
// extended-key flag) is set.
using px_key = uint32_t;

inline constexpr px_key PX_KEY_NAMED = 0x80000000u;

enum : px_key {
    PX_KEY_NONE = 0,

    PX_KEY_BACKSPACE = PX_KEY_NAMED | 0x08,
    PX_KEY_TAB = PX_KEY_NAMED | 0x09,
    PX_KEY_ENTER = PX_KEY_NAMED | 0x0D,
    PX_KEY_PAUSE = PX_KEY_NAMED | 0x13,
    PX_KEY_ESCAPE = PX_KEY_NAMED | 0x1B,
    PX_KEY_SPACE = PX_KEY_NAMED | 0x20,
    PX_KEY_PAGE_UP = PX_KEY_NAMED | 0x21,
    PX_KEY_PAGE_DOWN = PX_KEY_NAMED | 0x22,
    PX_KEY_END = PX_KEY_NAMED | 0x23,
    PX_KEY_HOME = PX_KEY_NAMED | 0x24,
    PX_KEY_LEFT = PX_KEY_NAMED | 0x25,
    PX_KEY_UP = PX_KEY_NAMED | 0x26,
    PX_KEY_RIGHT = PX_KEY_NAMED | 0x27,
    PX_KEY_DOWN = PX_KEY_NAMED | 0x28,
    PX_KEY_INSERT = PX_KEY_NAMED | 0x2D,
    PX_KEY_DELETE = PX_KEY_NAMED | 0x2E,

    PX_KEY_F1 = PX_KEY_NAMED | 0x70,
    PX_KEY_F2 = PX_KEY_NAMED | 0x71,
    PX_KEY_F3 = PX_KEY_NAMED | 0x72,
    PX_KEY_F4 = PX_KEY_NAMED | 0x73,
    PX_KEY_F5 = PX_KEY_NAMED | 0x74,
    PX_KEY_F6 = PX_KEY_NAMED | 0x75,
    PX_KEY_F7 = PX_KEY_NAMED | 0x76,
    PX_KEY_F8 = PX_KEY_NAMED | 0x77,
    PX_KEY_F9 = PX_KEY_NAMED | 0x78,
    PX_KEY_F10 = PX_KEY_NAMED | 0x79,
    PX_KEY_F11 = PX_KEY_NAMED | 0x7A,
    PX_KEY_F12 = PX_KEY_NAMED | 0x7B,

    // Beyond the VK space: synthetic keys ST invents for positions Win32 folds together.
    PX_KEY_KEYPAD_ENTER = PX_KEY_NAMED | 0x10F,
};

// Modifier bitmask. ST tests these as a group (`mods & 0x3e0`), implying its own flags occupy the
// low bits and the modifier group sits above them; the exact bit assignment is ours.
enum : uint32_t {
    PX_MOD_SHIFT = 1u << 0,
    PX_MOD_CONTROL = 1u << 1,
    PX_MOD_ALT = 1u << 2,    // Option on macOS
    PX_MOD_SUPER = 1u << 3,  // Command on macOS
    PX_MOD_CAPS_LOCK = 1u << 4,
};

enum px_mouse_button : int {
    PX_MOUSE_NONE = 0,
    PX_MOUSE_LEFT = 1,
    PX_MOUSE_RIGHT = 2,
    PX_MOUSE_MIDDLE = 3,
    PX_MOUSE_X1 = 4,
    PX_MOUSE_X2 = 5,
};

enum px_cursor_t : int {
    PX_CURSOR_ARROW = 0,
    PX_CURSOR_IBEAM = 1,
    PX_CURSOR_CROSSHAIR = 2,
    PX_CURSOR_POINTING_HAND = 3,
    PX_CURSOR_RESIZE_LEFT_RIGHT = 4,
    PX_CURSOR_RESIZE_UP_DOWN = 5,
};

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// EVENTS
// ─────────────────────────────────────────────────────────────────────────────────────────────────
// One tag space for every platform. The numbering is ST's, recovered from its Win32 WndProc jump
// tables and cross-checked against the Cocoa entry points.
enum px_event_type : int {
    // 0 is the value a memset leaves behind, and it is what ST's key path relies on: the
    // WM_KEYDOWN
    // arm never stores a tag at all.
    PX_EVENT_KEY = 0,
    PX_EVENT_CHARACTER = 1,          // WM_CHAR
    PX_EVENT_MOUSE_BUTTON = 2,       // WM_*BUTTONDOWN / UP / DBLCLK
    PX_EVENT_MOUSE_MOTION = 3,       // WM_MOUSEMOVE
    PX_EVENT_MOUSE_LEAVE = 4,        // WM_MOUSELEAVE
    PX_EVENT_SCROLL = 6,             // WM_HSCROLL / WM_VSCROLL
    PX_EVENT_CAPTURE_LOST = 7,       // WM_CAPTURECHANGED
    PX_EVENT_RESIZE = 8,             // WM_SIZE, -[PXWindowDelegate windowDidResize:]
    PX_EVENT_DESTROY = 9,            // WM_DESTROY
    PX_EVENT_DPI_CHANGED = 10,       // WM_DPICHANGED
    PX_EVENT_DROP_FILES = 11,        // WM_DROPFILES
    PX_EVENT_FOCUS_GAINED = 13,      // WM_SETFOCUS
    PX_EVENT_FOCUS_LOST = 14,        // WM_KILLFOCUS
    PX_EVENT_SETTINGS_CHANGED = 21,  // WM_SETTINGCHANGE
};

// A single POD carrying every input event. ST's is 0x158 bytes, zeroed with memset before each
// use, and passed by pointer straight through window_impl and the whole control tree --
// control::handle_event takes `px_event_t const*`, so it is never rewrapped into an app-level
// type.
//
// Fields are grouped by the tag that makes them meaningful rather than unioned, so a debugger
// print is readable; the whole struct is memset before every send either way.
struct px_event_t {
    px_event_type type = PX_EVENT_KEY;
    px_window_t* window = nullptr;

    // PX_EVENT_KEY / PX_EVENT_CHARACTER / all mouse events.
    uint32_t modifiers = 0;

    // PX_EVENT_KEY.
    px_key key = PX_KEY_NONE;
    bool pressed = false;
    bool repeat = false;

    // PX_EVENT_CHARACTER. UTF-8, NUL-terminated. Sized to hold a composed sequence comfortably;
    // ST's oversized event struct exists for the same reason.
    char text[64] = {};

    // Mouse events, in window-space points with a top-left origin.
    vec2 pos;
    px_mouse_button button = PX_MOUSE_NONE;
    int click_count = 0;

    // PX_EVENT_SCROLL. Positive dy scrolls the content up (finger moves up).
    vec2 scroll_delta;
    bool precise_scroll = false;

    // PX_EVENT_RESIZE / PX_EVENT_DPI_CHANGED.
    vec2 size;
    double dpi_scale_factor = 1.0;

    // PX_EVENT_DROP_FILES. Points at storage owned by the platform layer, valid only for the
    // duration of the handle_event call.
    const char* const* paths = nullptr;
    int path_count = 0;
};

// Fills px_event_t::text, truncating on a UTF-8 codepoint boundary.
//
// The field is a fixed array inside a memset POD, so unlike a scratch buffer it cannot be replaced
// by a std::string return: the struct is the thing that crosses the platform boundary. What it can
// do is truncate honestly. A plain bounded copy will cut a multi-byte sequence in half and leave
// invalid UTF-8 behind, which is not hypothetical on the IME path -- a committed CJK string is all
// three-byte sequences, and 64 bytes is only 21 of them.
//
// Returns false if the text did not fit.
inline bool px_set_event_text(px_event_t* event, const char* utf8, size_t length) {
    constexpr size_t kCapacity = sizeof(px_event_t::text) - 1;  // reserve the terminator

    size_t n = length < kCapacity ? length : kCapacity;
    if (n < length) {
        // Back off over any continuation bytes (10xxxxxx) so the cut lands on a lead byte.
        while (n > 0 && (static_cast<unsigned char>(utf8[n]) & 0xC0) == 0x80) {
            --n;
        }
    }
    for (size_t i = 0; i < n; ++i) {
        event->text[i] = utf8[i];
    }
    event->text[n] = '\0';
    return n == length;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// RENDER CONTEXT
// ─────────────────────────────────────────────────────────────────────────────────────────────────
// Handed to px_window_event_handler::paint. This is the backend-neutral 2D drawing interface; ST
// has both gl_render_context and skia_render_context implementations. The recovered interface is
// larger (text, gradients, paths and custom drawing), but those methods depend on px_font_t,
// fx_layout and texture types that are not present in this experiment. This is the faithful,
// functional solid-rectangle/state slice rather than a collection of no-op placeholders.
class px_render_context {
public:
    virtual ~px_render_context() = default;

    virtual void draw_rect(rect area, fill_mode fill) = 0;
    void draw_rect(rect area, fcolor color) { draw_rect(area, fill_mode(color)); }

    // Transform operations compose exactly as ST's GL implementation does: translation is first
    // multiplied by the current scale, while scale multiplies the current scale component-wise.
    virtual void translate(double x, double y) = 0;
    virtual void scale(double x, double y) = 0;

    // Intersects the current device-pixel clip with a transformed logical rectangle and applies it
    // immediately. push_state(false) isolates an active batch, matching the behavior visible in
    // gl_render_context::push_state(bool); true leaves the current batch active across the save.
    virtual void restrict_clip_rect(rect area) = 0;
    virtual void push_state(bool preserve_batch) = 0;
    virtual void pop_state() = 0;

    virtual vec2 get_translation() = 0;
    virtual vec2 get_scale() = 0;
    virtual recti get_clip_rect() = 0;
    virtual double dpi_scale_factor() = 0;

    virtual bool supports_batching() const { return false; }
    virtual void begin_rect_batch() {}
    virtual void end_rect_batch() {}
};

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// THE ONE INBOUND INTERFACE
// ─────────────────────────────────────────────────────────────────────────────────────────────────
// Mirrors ST's px_window_event_handler exactly: 13 virtuals plus the destructor, in vtable order.
// Every method is per-category. All input funnels through handle_event.
class px_window_event_handler {
public:
    virtual ~px_window_event_handler() = default;

    // Returns true if the event was consumed.
    virtual bool handle_event(px_event_t* event) = 0;

    // The GL context is current and the framebuffer is bound. `dirty` lists the regions Core
    // Animation asked for, in window-space points.
    virtual void paint(px_render_context* rc, rect bounds, const rect* dirty, int dirty_count) = 0;

    // Called immediately before a paint, and after an input burst settles. Where ST reconciles
    // layout so paint() can stay pure.
    virtual void pre_paint() {}

    // The event loop is about to block. Last chance to flush lazy work.
    virtual void pre_sleep() {}

    // Fast path for quit: may the window close without asking the user anything?
    virtual bool can_close_without_prompt() { return true; }

    // Slow path. Prompt if needed, then report. ST's signature uses a move-only function; this is
    // the one place callbacks appear in the inbound interface, and it is an async completion.
    virtual void try_close(std::function<void(bool)> done) { done(true); }

    // Returns the cursor for a window-space point. Called from -[PXView resetCursorRects].
    virtual px_cursor_t calculate_cursor(vec2 pos) {
        (void)pos;
        return PX_CURSOR_ARROW;
    }

    virtual bool drag_drop_enter(vec2 pos, const char* const* paths, int count) {
        (void)pos;
        (void)paths;
        (void)count;
        return false;
    }
    virtual bool drag_drop_motion(vec2 pos, const char* const* paths, int count) {
        (void)pos;
        (void)paths;
        (void)count;
        return false;
    }
    virtual void drag_drop_exit() {}
    virtual bool drag_drop_accept(vec2 pos, const char* const* paths, int count) {
        (void)pos;
        (void)paths;
        (void)count;
        return false;
    }

    // Driven by the window's CVDisplayLink. `now` is seconds since px_init.
    virtual void animation_tick(double now) { (void)now; }

    // The IME target, or null if this window takes no text input.
    virtual px_input_client* get_input_client() { return nullptr; }
};

// A no-op implementation, so a px_window_t never holds a null handler. ST ships the same thing as
// dummy_px_window_event_handler.
class dummy_px_window_event_handler : public px_window_event_handler {
public:
    bool handle_event(px_event_t*) override { return false; }
    void paint(px_render_context*, rect, const rect*, int) override {}
};

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// IME
// ─────────────────────────────────────────────────────────────────────────────────────────────────
// PXView conforms to NSTextInputClient and forwards to whatever get_input_client() returns. ST's
// px_input_client is 19 virtuals; this is the subset NSTextInputClient actually requires.
// Ranges are UTF-16 offsets, because that is the currency NSTextInputClient speaks.
struct px_range_t {
    int64_t location = -1;  // -1 means NSNotFound
    int64_t length = 0;

    static px_range_t none() { return px_range_t{-1, 0}; }
    bool valid() const { return location >= 0; }
};

class px_input_client {
public:
    virtual ~px_input_client() = default;

    // Committed text. `replacement` may be invalid, meaning "replace the current selection".
    virtual void insert_text(const char* utf8, px_range_t replacement) = 0;

    // In-progress composition.
    virtual void set_marked_text(const char* utf8,
                                 px_range_t selected,
                                 px_range_t replacement) = 0;
    virtual void unmark_text() = 0;
    virtual bool has_marked_text() const = 0;
    virtual px_range_t marked_range() const = 0;
    virtual px_range_t selected_range() const = 0;

    // Window-space rect (points) of the given range, for positioning the candidate window.
    virtual rect first_rect_for_range(px_range_t range, px_range_t* actual) = 0;

    // Inverse mapping, for mouse-driven candidate selection.
    virtual int64_t character_index_for_point(vec2 pos) = 0;

    // Editor-side commands the input method requests by selector (e.g. "moveLeft:").
    virtual void do_command(const char* selector_name) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// APPLICATION-LEVEL INBOUND INTERFACE
// ─────────────────────────────────────────────────────────────────────────────────────────────────
// ST's px_application_event_handler, implemented there by `window_list`.
class px_application_event_handler {
public:
    virtual ~px_application_event_handler() = default;

    virtual void open_files(const char* const* paths, int count) {
        (void)paths;
        (void)count;
    }
    virtual void new_file() {}
    virtual bool can_quit_without_prompt() { return true; }
    virtual void try_quit(std::function<void(bool)> done) { done(true); }
    virtual void appearance_changed() {}
};

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// APP -> OS: FLAT C API OVER OPAQUE HANDLES
// ─────────────────────────────────────────────────────────────────────────────────────────────────
enum : uint32_t {
    PX_WINDOW_RESIZABLE = 1u << 0,
    PX_WINDOW_TITLED = 1u << 1,
    PX_WINDOW_CLOSABLE = 1u << 2,
    PX_WINDOW_MINIATURIZABLE = 1u << 3,

    // Use AppKit's ordinary layer-backed NSView rather than CAOpenGLLayer. Sublime's 0x119 drag
    // helper omits its 0x20000 OpenGL-enable flag, so its cached texture window takes this path
    // even though the editor's main windows use GL.
    PX_WINDOW_SOFTWARE = 1u << 16,

    // Native surface properties used by Sublime's internal drag window. These are creation flags,
    // rather than post-creation mutations, because AppKit chooses backing/window-server behavior
    // while the content view is attached.
    PX_WINDOW_POPUP_LEVEL = 1u << 17,
    PX_WINDOW_TRANSPARENT = 1u << 18,

    PX_WINDOW_DEFAULT =
        PX_WINDOW_RESIZABLE | PX_WINDOW_TITLED | PX_WINDOW_CLOSABLE | PX_WINDOW_MINIATURIZABLE,
};

// Set PX_NO_GL=1 in the environment to take the software path, mirroring the pxw->use_gl flag that
// -[PXView makeBackingLayer] branches on.
void px_init(const char* app_name, const char* bundle_id, int argc, char** argv, uint32_t flags);
void px_set_application_event_handler(px_application_event_handler* handler);
void px_run_event_loop();
void px_exit_event_loop();

// The handler must outlive the window. Passing null installs the dummy handler.
px_window_t* px_create_window(px_window_event_handler* handler,
                              px_window_t* parent,
                              double width,
                              double height,
                              const char* title,
                              fcolor background,
                              uint32_t flags);
void px_destroy_window(px_window_t* window);

void px_show_window(px_window_t* window);
void px_show_window_without_focus(px_window_t* window);
void px_hide_window(px_window_t* window);
void px_close_window(px_window_t* window);

void px_set_window_title(px_window_t* window, const char* title);
vec2 px_window_size(px_window_t* window);
vec2 px_window_position(px_window_t* window);
void px_set_window_size(px_window_t* window, double width, double height);
void px_set_window_position(px_window_t* window, vec2 position);
double px_window_dpi_scale_factor(px_window_t* window);
void px_set_full_screen(px_window_t* window, bool full_screen);

// Accumulates into the window's dirty list. Flushed to setNeedsDisplayInRect: after the current
// event settles, exactly as ST's flush_dirty_rects does.
void px_mark_rect_dirty(px_window_t* window, rect r);
void px_mark_dirty(px_window_t* window);

void px_set_cursor(px_window_t* window, px_cursor_t cursor);
void px_reset_cursor(px_window_t* window);

// Runs `fn` after the current event finishes being handled, on the main thread. ST calls the
// drain point dispatch_post_event_callbacks() and invokes it from both send_event and the layer's
// draw callback.
void px_callback_after_event(std::function<void()> fn);
void px_set_timeout(std::function<void()> fn, int milliseconds);

bool px_os_in_dark_mode();
double px_caret_blink_time();  // seconds; 0 means "do not blink"
void px_show_error(px_window_t* parent, const char* message);
void px_open_url(const char* url);

// Seconds since px_init. The clock animation_tick is stamped from.
double px_now();
