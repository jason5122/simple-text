// The app-side half of the pattern: portable, no platform headers.
//
// ST's layering, from the Windows RTTI:
//
//   class window_impl : public window,                    // mdisp=0, 26 virtuals
//                       public px_window_event_handler;   // mdisp=8, 14 virtuals
//
// window_impl::handle_event does not switch on the event tag. It walks a vector<window_aspect*> as
// a chain of responsibility: each aspect's handle_event returns whether it consumed the event, and
// once one has, the remaining aspects receive notify_event instead so they can still observe.
// Decoded at 0x140197e22:
//
//     for (aspect : aspects)
//         if (handled) aspect->vfunc[2](e);           // notify_event
//         else         handled = aspect->vfunc[1](e); // handle_event -> bool
//
// That is how ST avoids growing `window` a subclass per behaviour: hover tracking, tooltips, the
// input automaton and the pointer automaton are all aspects (window_hover_aspect,
// window_tooltip_aspect, window_input_automata_aspect, window_pointer_automata_aspect).

#pragma once

#include <functional>
#include <vector>

#include "experiments/platform/px/px.h"

class window;

// 3 virtuals, matching the aspect vtable size in the binary.
class window_aspect {
public:
    virtual ~window_aspect() = default;

    // Returns true to consume the event.
    virtual bool handle_event(px_event_t* event) = 0;

    // Called instead of handle_event once some earlier aspect has consumed it.
    virtual void notify_event(px_event_t* event) = 0;
};

// The window's owner. ST's window_handler is 3 methods plus the destructor; window_tooltip_aspect
// implements it as a secondary base at mdisp=8.
class window_handler {
public:
    virtual ~window_handler() = default;
    virtual bool can_close_without_prompt(window* w) = 0;
    virtual void on_close(window* w) = 0;
    virtual void try_close(window* w, std::function<void(bool)> done) = 0;
};

// The attach point for a control tree. ST's is `control`, whose merged vtable with message_handler
// runs to 61 entries; the only part that matters at this boundary is that it takes the platform's
// event struct unchanged -- control::handle_event(px_event_t const*) -- and is never handed a
// rewrapped app-level event type.
class control {
public:
    virtual ~control() = default;
    virtual bool handle_event(const px_event_t* event) = 0;
    virtual void draw(px_render_context* rc, rect bounds, const rect* dirty, int dirty_count) = 0;
};

// Portable window interface. Deliberately not a base class you subclass to get behaviour -- it is
// the app's handle on a platform window, and behaviour is added with aspects.
class window {
public:
    virtual ~window() = default;

    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void close() = 0;
    virtual void set_title(const char* title) = 0;
    virtual void set_maximized(bool maximized) = 0;
    virtual vec2 size() const = 0;
    virtual double dpi_scale_factor() const = 0;

    virtual void mark_dirty() = 0;
    virtual void mark_rect_dirty(rect r) = 0;

    virtual void add_window_aspect(window_aspect* aspect) = 0;
    virtual void set_root_control(control* root) = 0;
    virtual void set_handler(window_handler* handler) = 0;
    virtual void set_input_client(px_input_client* client) = 0;

    virtual px_window_t* px_window() const = 0;
};

// The bridge. This is the only class in the app that the platform layer knows about.
class window_impl : public window, public px_window_event_handler {
public:
    window_impl(double width, double height, const char* title, fcolor background);
    ~window_impl() override;

    window_impl(const window_impl&) = delete;
    window_impl& operator=(const window_impl&) = delete;

    // window
    void show() override;
    void hide() override;
    void close() override;
    void set_title(const char* title) override;
    void set_maximized(bool maximized) override;
    vec2 size() const override;
    double dpi_scale_factor() const override;
    void mark_dirty() override;
    void mark_rect_dirty(rect r) override;
    void add_window_aspect(window_aspect* aspect) override;
    void set_root_control(control* root) override;
    void set_handler(window_handler* handler) override;
    void set_input_client(px_input_client* client) override;
    px_window_t* px_window() const override { return px_window_; }

    // px_window_event_handler
    bool handle_event(px_event_t* event) override;
    void paint(px_render_context* rc, rect bounds, const rect* dirty, int dirty_count) override;
    void pre_paint() override;
    bool can_close_without_prompt() override;
    void try_close(std::function<void(bool)> done) override;
    px_cursor_t calculate_cursor(vec2 pos) override;
    void animation_tick(double now) override;
    px_input_client* get_input_client() override { return input_client_; }

private:
    px_window_t* px_window_ = nullptr;
    std::vector<window_aspect*> aspects_;
    control* root_ = nullptr;
    window_handler* handler_ = nullptr;
    px_input_client* input_client_ = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// STOCK ASPECTS
// ─────────────────────────────────────────────────────────────────────────────────────────────────

// Keeps the window's own bookkeeping current: size, scale, focus. Consumes nothing, so it always
// sees every event. ST installs its equivalent first for the same reason.
class window_basic_aspect final : public window_aspect {
public:
    explicit window_basic_aspect(window* w) : window_(w) {}

    bool handle_event(px_event_t* event) override;
    void notify_event(px_event_t* event) override;

    vec2 size() const { return size_; }
    double dpi_scale_factor() const { return dpi_scale_factor_; }
    bool focused() const { return focused_; }

private:
    void observe(const px_event_t* event);

    window* window_ = nullptr;
    vec2 size_;
    double dpi_scale_factor_ = 1.0;
    bool focused_ = false;
};

// Tracks the pointer so hover-sensitive drawing has somewhere to read from.
class window_hover_aspect final : public window_aspect {
public:
    explicit window_hover_aspect(window* w) : window_(w) {}

    bool handle_event(px_event_t* event) override;
    void notify_event(px_event_t* event) override;

    bool inside() const { return inside_; }
    vec2 pos() const { return pos_; }
    double pos_time() const { return pos_time_; }  // px_now() when pos_ was last written

private:
    void observe(const px_event_t* event);

    window* window_ = nullptr;
    vec2 pos_;
    bool inside_ = false;
    double pos_time_ = 0.0;
};
