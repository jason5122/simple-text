#include "experiments/platform/window.h"

#include <algorithm>

window_impl::window_impl(double width, double height, const char* title, fcolor background) {
  // The handler goes in at construction, the way px_create_window takes it as its first argument.
  px_window_ = px_create_window(this, nullptr, width, height, title, background, PX_WINDOW_DEFAULT);
}

window_impl::~window_impl() {
  if (px_window_) {
    px_destroy_window(px_window_);
    px_window_ = nullptr;
  }
}

// ── window ──────────────────────────────────────────────────────────────────────────────────────

void window_impl::show() {
  px_show_window(px_window_);
}

void window_impl::hide() {
  px_hide_window(px_window_);
}

void window_impl::close() {
  px_close_window(px_window_);
}

void window_impl::set_title(const char* title) {
  px_set_window_title(px_window_, title);
}

vec2 window_impl::size() const {
  return px_window_size(px_window_);
}

double window_impl::dpi_scale_factor() const {
  return px_window_dpi_scale_factor(px_window_);
}

void window_impl::mark_dirty() {
  px_mark_dirty(px_window_);
}

void window_impl::mark_rect_dirty(rect r) {
  px_mark_rect_dirty(px_window_, r);
}

void window_impl::add_window_aspect(window_aspect* aspect) {
  if (aspect) {
    aspects_.push_back(aspect);
  }
}

void window_impl::set_root_control(control* root) {
  root_ = root;
}

void window_impl::set_handler(window_handler* handler) {
  handler_ = handler;
}

void window_impl::set_input_client(px_input_client* client) {
  input_client_ = client;
}

// ── px_window_event_handler ─────────────────────────────────────────────────────────────────────

bool window_impl::handle_event(px_event_t* event) {
  // The chain, transcribed from 0x140197e22. Note there is no switch on event->type here: routing
  // by tag is each aspect's business, and the control tree's.
  bool handled = false;
  for (window_aspect* aspect : aspects_) {
    if (handled) {
      aspect->notify_event(event);
    } else {
      handled = aspect->handle_event(event);
    }
  }

  if (!handled && root_) {
    handled = root_->handle_event(event);
  }
  return handled;
}

void window_impl::paint(px_render_context* rc, rect bounds, const rect* dirty, int dirty_count) {
  if (root_) {
    root_->draw(rc, bounds, dirty, dirty_count);
  }
}

void window_impl::pre_paint() {
  // Where layout would be reconciled, so paint() can stay free of side effects.
}

bool window_impl::can_close_without_prompt() {
  return handler_ ? handler_->can_close_without_prompt(this) : true;
}

void window_impl::try_close(std::function<void(bool)> done) {
  if (!handler_) {
    done(true);
    return;
  }
  handler_->try_close(this, std::move(done));
}

px_cursor_t window_impl::calculate_cursor(vec2 pos) {
  (void)pos;
  return PX_CURSOR_ARROW;
}

void window_impl::animation_tick(double now) {
  (void)now;
}

// ── stock aspects ───────────────────────────────────────────────────────────────────────────────

void window_basic_aspect::observe(const px_event_t* event) {
  switch (event->type) {
    case PX_EVENT_RESIZE:
      size_ = event->size;
      dpi_scale_factor_ = event->dpi_scale_factor;
      break;
    case PX_EVENT_DPI_CHANGED:
      dpi_scale_factor_ = event->dpi_scale_factor;
      break;
    case PX_EVENT_FOCUS_GAINED:
      focused_ = true;
      break;
    case PX_EVENT_FOCUS_LOST:
      focused_ = false;
      break;
    default:
      break;
  }
}

bool window_basic_aspect::handle_event(px_event_t* event) {
  observe(event);
  // Consumes nothing: bookkeeping must not stop an event reaching the control tree.
  return false;
}

void window_basic_aspect::notify_event(px_event_t* event) {
  observe(event);
}

void window_hover_aspect::observe(const px_event_t* event) {
  switch (event->type) {
    case PX_EVENT_MOUSE_MOTION:
      pos_ = event->pos;
      pos_time_ = px_now();
      inside_ = true;
      break;
    case PX_EVENT_MOUSE_BUTTON:
      pos_ = event->pos;
      pos_time_ = px_now();
      break;
    case PX_EVENT_MOUSE_LEAVE:
      inside_ = false;
      break;
    default:
      break;
  }
}

bool window_hover_aspect::handle_event(px_event_t* event) {
  observe(event);
  // Pure bookkeeping, like window_basic_aspect: this aspect has no idea what a control draws at
  // the hover position or how large it is, so it has no business deciding how much of the window
  // needs to repaint. That decision belongs to whichever control actually draws something there --
  // the same division of labor ST's drag_resizer relies on, marking only its own control dirty
  // rather than reaching for a window-wide invalidation on every mouse-move sample.
  return false;
}

void window_hover_aspect::notify_event(px_event_t* event) {
  observe(event);
}
