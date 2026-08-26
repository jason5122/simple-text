// Exercises the whole path: px_create_window -> PXView -> send_event -> window_impl::handle_event
// -> aspect chain -> control, and CAOpenGLLayer -> paint.
//
// Events are logged with their numeric tag so a trace from this binary can be lined up against one
// taken from the platform being mirrored.

#include <algorithm>
#include <cmath>
#include <cstdio>   // std::setvbuf
#include <cstdlib>  // getenv
#include <cstring>
#include <format>
#include <print>
#include <string>
#include <vector>

#if defined(PX_DEMO_FENCE_RECT_BATCH)
#include "experiments/platform/demo/fence_rect_batch.h"
#else
#include "experiments/platform/demo/rect_batch.h"
#endif
#include "experiments/platform/px.h"
#include "experiments/platform/px_gl.h"
#include "experiments/platform/window.h"

namespace {

#if defined(PX_DEMO_FENCE_RECT_BATCH)
using DemoRectBatch = px_demo::FenceRectBatch;
constexpr const char* kDemoAppName = "platform fence";
constexpr const char* kDemoTitle = "platform - one-frame fence demo";
#else
using DemoRectBatch = px_demo::RectBatch;
constexpr const char* kDemoAppName = "platform";
constexpr const char* kDemoTitle = "platform - texture-buffer demo";
#endif

const char* event_name(px_event_type type) {
  switch (type) {
    case PX_EVENT_KEY: return "key";
    case PX_EVENT_CHARACTER: return "character";
    case PX_EVENT_MOUSE_BUTTON: return "mouse_button";
    case PX_EVENT_MOUSE_MOTION: return "mouse_motion";
    case PX_EVENT_MOUSE_LEAVE: return "mouse_leave";
    case PX_EVENT_SCROLL: return "scroll";
    case PX_EVENT_CAPTURE_LOST: return "capture_lost";
    case PX_EVENT_RESIZE: return "resize";
    case PX_EVENT_DESTROY: return "destroy";
    case PX_EVENT_DPI_CHANGED: return "dpi_changed";
    case PX_EVENT_DROP_FILES: return "drop_files";
    case PX_EVENT_FOCUS_GAINED: return "focus_gained";
    case PX_EVENT_FOCUS_LOST: return "focus_lost";
    case PX_EVENT_SETTINGS_CHANGED: return "settings_changed";
  }
  return "?";
}

std::string describe_key(px_key key) {
  if (key == PX_KEY_NONE) {
    return "none";
  }
  if (key & PX_KEY_NAMED) {
    return std::format("named:{:#x}", key & ~PX_KEY_NAMED);
  }
  if (key < 0x80) {
    return std::format("'{}'", static_cast<char>(key));
  }
  return std::format("u+{:04x}", key);
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// The demo's control: the stand-in for what would be a real control tree.
// ─────────────────────────────────────────────────────────────────────────────────────────────────

class DemoControl final : public control, public px_input_client {
 public:
  DemoControl(window* w, const window_hover_aspect* hover) : window_(w), hover_(hover) {}

  void set_phase(double phase) { phase_ = phase; }

  // ── control ───────────────────────────────────────────────────────────────────────────────────

  bool handle_event(const px_event_t* event) override {
    // px_event_type is an enum, and std::format has no formatter for enums the way printf's %d
    // accepted them by promotion, so the tag is cast at every site.
    const int tag = static_cast<int>(event->type);

    switch (event->type) {
      case PX_EVENT_KEY:
        std::println("[{:2} {:<16}] key={} mods={:#x} {}{}", tag, event_name(event->type),
                     describe_key(event->key), event->modifiers,
                     event->pressed ? "down" : "up", event->repeat ? " repeat" : "");
        // Escape closes, to prove an app-consumed key never reaches the input context.
        if (event->pressed && event->key == PX_KEY_ESCAPE) {
          window_->close();
          return true;
        }
        break;

      case PX_EVENT_CHARACTER:
        std::println("[{:2} {:<16}] text=\"{}\"", tag, event_name(event->type), event->text);
        break;

      case PX_EVENT_MOUSE_BUTTON:
        std::println("[{:2} {:<16}] button={} {} clicks={} at ({:.1f}, {:.1f})", tag,
                     event_name(event->type), static_cast<int>(event->button),
                     event->pressed ? "down" : "up", event->click_count, event->pos.x,
                     event->pos.y);
        break;

      case PX_EVENT_SCROLL:
        // A trackpad can deliver thousands of packets during a short gesture. Keep unbuffered
        // terminal I/O out of the smoothness test unless the event trace is explicitly wanted.
        if (getenv("PX_SCROLL_LOG")) {
          std::println("[{:2} {:<16}] delta=({:.2f}, {:.2f}) precise={}", tag,
                       event_name(event->type), event->scroll_delta.x, event->scroll_delta.y,
                       event->precise_scroll ? 1 : 0);
        }
        // px's positive scroll delta means that content moves up. Keep this as a logical viewport
        // offset and apply the sign once while drawing; retaining the fractional value is useful
        // for judging trackpad cadence and sub-pixel motion.
        scroll_offset_ = std::clamp(scroll_offset_ - event->scroll_delta.y, 0.0,
                                    kDocumentHeight - 120.0);
        window_->mark_dirty();
        break;

      case PX_EVENT_RESIZE:
        std::println("[{:2} {:<16}] size=({:.0f}, {:.0f}) scale={:.2f}", tag,
                     event_name(event->type), event->size.x, event->size.y,
                     event->dpi_scale_factor);
        break;

      case PX_EVENT_DPI_CHANGED:
        std::println("[{:2} {:<16}] scale={:.2f}", tag, event_name(event->type),
                     event->dpi_scale_factor);
        break;

      case PX_EVENT_DROP_FILES:
        std::println("[{:2} {:<16}] {} path(s)", tag, event_name(event->type), event->path_count);
        for (int i = 0; i < event->path_count; ++i) {
          std::println("    {}", event->paths[i]);
        }
        break;

      // Motion is far too chatty to log.
      case PX_EVENT_MOUSE_MOTION:
        mark_hover_square_dirty(event->pos);
        break;

      case PX_EVENT_MOUSE_LEAVE:
        mark_hover_square_dirty(last_hover_pos_);
        break;

      default:
        std::println("[{:2} {:<16}]", tag, event_name(event->type));
        break;
    }
    return false;
  }

  void draw(px_render_context* rc, rect bounds, const rect* dirty, int dirty_count) override {
    batch_.ensure_initialized();

    // Honour the dirty list with the scissor box. This is what the persistent drawable buys on
    // both platforms -- kCGLPFABackingStore on macOS, a single-buffered pixel format on Windows:
    // pixels outside the dirty region survive from the previous frame.
    glEnable(GL_SCISSOR_TEST);

    int union_box[4] = {};
    bool have_union = false;
    for (int i = 0; i < dirty_count; ++i) {
      int box[4] = {};
      rc->scissor_box(dirty[i], box);
      if (box[2] <= 0 || box[3] <= 0) {
        continue;
      }
      glScissor(box[0], box[1], box[2], box[3]);
      glClearColor(0.09f, 0.10f, 0.12f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      if (!have_union) {
        std::memcpy(union_box, box, sizeof(box));
        have_union = true;
      } else {
        const int right = std::max(union_box[0] + union_box[2], box[0] + box[2]);
        const int top = std::max(union_box[1] + union_box[3], box[1] + box[3]);
        union_box[0] = std::min(union_box[0], box[0]);
        union_box[1] = std::min(union_box[1], box[1]);
        union_box[2] = right - union_box[0];
        union_box[3] = top - union_box[1];
      }
    }
    if (!have_union) {
      glDisable(GL_SCISSOR_TEST);
      return;
    }
    // Geometry is clipped to the bounding box of the dirty region, so an incremental repaint
    // genuinely touches only the pixels it claimed.
    glScissor(union_box[0], union_box[1], union_box[2], union_box[3]);

    // A grid, so resizing and dirty-rect behaviour is visible.
    constexpr double kCell = 48.0;
    for (double y = 0; y < bounds.h; y += kCell) {
      for (double x = 0; x < bounds.w; x += kCell) {
        const bool alternate = static_cast<int>(x / kCell + y / kCell) % 2 == 0;
        const float shade = alternate ? 0.14f : 0.17f;
        batch_.add(rect{x + 1, y + 1, kCell - 2, kCell - 2}, fcolor{shade, shade, shade + 0.02f, 1});
      }
    }

    // A long static document translated only by scroll input. Thin rules and repeated high-
    // contrast edges make uneven frame cadence much easier to see than the old single bar. Cull
    // rows outside the viewport as a real editor would, while leaving a little overscan at either
    // edge.
    const double document_y = 72.0 - scroll_offset_;
    const double document_width = std::max(280.0, bounds.w - 128.0);
    batch_.add(rect{48.0, document_y, document_width, kDocumentHeight},
               fcolor{0.105f, 0.115f, 0.135f, 1.0f});

    constexpr double kRowPitch = 64.0;
    constexpr double kRowHeight = 48.0;
    constexpr int kRowCount = static_cast<int>(kDocumentHeight / kRowPitch);
    for (int row = 0; row < kRowCount; ++row) {
      const double y = document_y + 24.0 + row * kRowPitch;
      if (y + kRowHeight < -kRowPitch || y > bounds.h + kRowPitch) {
        continue;
      }

      const bool section = row % 8 == 0;
      const float shade = row % 2 == 0 ? 0.175f : 0.145f;
      batch_.add(rect{68.0, y, document_width - 40.0, kRowHeight},
                 fcolor{shade, shade + 0.008f, shade + 0.025f, 1.0f});
      batch_.add(rect{68.0, y, section ? 7.0 : 3.0, kRowHeight},
                 section ? fcolor{0.30f, 0.68f, 0.93f, 1.0f}
                         : fcolor{0.31f, 0.36f, 0.43f, 1.0f});

      // Fake glyph runs: deliberately varied lengths give the eye stable landmarks without
      // requiring the text renderer this demo is eventually intended to exercise.
      const double title_width = 105.0 + (row * 37 % 190);
      const double detail_width = 70.0 + (row * 53 % 260);
      batch_.add(rect{88.0, y + 10.0, title_width, 5.0},
                 section ? fcolor{0.70f, 0.84f, 0.96f, 1.0f}
                         : fcolor{0.63f, 0.66f, 0.72f, 1.0f});
      batch_.add(rect{88.0, y + 25.0, detail_width, 3.0},
                 fcolor{0.37f, 0.41f, 0.48f, 1.0f});
      batch_.add(rect{88.0, y + 36.0, document_width - 72.0, 1.0},
                 fcolor{0.235f, 0.255f, 0.295f, 1.0f});

      const float marker_r = 0.36f + static_cast<float>((row * 29) % 25) / 100.0f;
      const float marker_g = 0.40f + static_cast<float>((row * 17) % 22) / 100.0f;
      batch_.add(rect{document_width + 18.0, y + 15.0, 10.0, 18.0},
                 fcolor{marker_r, marker_g, 0.72f, 1.0f});
    }

    // A bar driven by animation_tick, to show the display link is live. Its vertical position is
    // fixed to the viewport so it remains separate from the static scroll test above.
    const double sweep = (std::sin(phase_) * 0.5 + 0.5) * std::max(0.0, bounds.w - 120.0);
    batch_.add(rect{sweep, 12.0, 120.0, 10.0}, fcolor{0.35f, 0.65f, 0.95f, 1.0f});

    // A square under the cursor, fed by the hover aspect rather than by this control reading
    // events, to show the aspect chain doing its job.
    if (hover_ && hover_->inside()) {
      const vec2 p = hover_->pos();
      batch_.add(rect{p.x - 10, p.y - 10, 20, 20}, fcolor{0.95f, 0.55f, 0.25f, 1.0f});
      // Keep terminal I/O out of the render path by default. This old event-to-draw probe excludes
      // CA presentation time; PX_LAG_TRACE enables the native event-to-target trace in the layer.
      if (getenv("PX_AGE_LOG")) {
        std::fprintf(stderr, "AGE %.2f\n", (px_now() - hover_->pos_time()) * 1000.0);
      }
    }

    batch_.flush(vec2{bounds.w, bounds.h});
    glDisable(GL_SCISSOR_TEST);
  }

  // ── px_input_client ───────────────────────────────────────────────────────────────────────────
  // Enough of NSTextInputClient to prove the IME path is wired. No text rendering, so committed
  // text is logged rather than shown.

  void insert_text(const char* utf8, px_range_t replacement) override {
    std::println("[ime] insert_text \"{}\" replacing [{},{})", utf8, replacement.location,
                 replacement.length);
    committed_ += utf8;
    marked_.clear();
    window_->mark_dirty();
  }

  void set_marked_text(const char* utf8, px_range_t selected, px_range_t replacement) override {
    (void)selected;
    (void)replacement;
    marked_ = utf8;
    std::println("[ime] marked \"{}\"", marked_);
  }

  void unmark_text() override { marked_.clear(); }
  bool has_marked_text() const override { return !marked_.empty(); }

  px_range_t marked_range() const override {
    if (marked_.empty()) {
      return px_range_t::none();
    }
    return px_range_t{static_cast<int64_t>(committed_.size()),
                      static_cast<int64_t>(marked_.size())};
  }

  px_range_t selected_range() const override {
    return px_range_t{static_cast<int64_t>(committed_.size()), 0};
  }

  rect first_rect_for_range(px_range_t range, px_range_t* actual) override {
    (void)range;
    if (actual) {
      *actual = px_range_t{0, 0};
    }
    // Where the candidate window should appear.
    return rect{12.0, 40.0, 1.0, 18.0};
  }

  int64_t character_index_for_point(vec2 pos) override {
    (void)pos;
    return 0;
  }

  void do_command(const char* selector_name) override {
    std::println("[ime] do_command {}", selector_name);
  }

 private:
  // Covers the square drawn at hover_->pos() (see draw()). Marking only the old and new footprint,
  // rather than the whole window, is what keeps a mouse-move sample cheap -- the same division of
  // labor drag_resizer relies on: touch just the pixels that actually changed, and let the
  // dirty-rect scissor in draw() do the rest. Both rects are needed: the new one so the square
  // appears at its current position, the old one so it's actually erased from its previous one --
  // skip it and every past position keeps a stale copy of the square baked into the framebuffer.
  void mark_hover_square_dirty(vec2 p) {
    constexpr double kHalf = 10.0;
    window_->mark_rect_dirty(
        rect{last_hover_pos_.x - kHalf, last_hover_pos_.y - kHalf, kHalf * 2, kHalf * 2});
    window_->mark_rect_dirty(rect{p.x - kHalf, p.y - kHalf, kHalf * 2, kHalf * 2});
    last_hover_pos_ = p;
  }

  window* window_ = nullptr;
  const window_hover_aspect* hover_ = nullptr;
  DemoRectBatch batch_;
  static constexpr double kDocumentHeight = 5200.0;
  double phase_ = 0.0;
  double scroll_offset_ = 0.0;
  vec2 last_hover_pos_;
  std::string committed_;
  std::string marked_;
};

// A window_impl subclass only to route animation_tick into the control. ST does the equivalent by
// keeping a list of animating controls on window_impl and ticking them from the display link.
class DemoWindow final : public window_impl {
 public:
  using window_impl::window_impl;

  void set_control(DemoControl* c) { control_ = c; }

  void animation_tick(double now) override {
    // PX_NO_ANIMATION: stop the continuous per-vsync mark_dirty() this drives, so the only
    // remaining source of redraws is discrete input events (mouse move, resize, scroll). A/B test
    // for whether continuous animation-driven redraw is what pushes CAOpenGLLayer/WindowServer
    // into a steady-state pipelined mode with several frames in flight -- fixed extra latency that
    // wouldn't show up as growing staleness in the input data itself, only as everything on screen
    // being consistently a few frames behind.
    if (getenv("PX_NO_ANIMATION")) {
      return;
    }
    if (!control_) {
      return;
    }
    control_->set_phase(now * 1.5);
    mark_dirty();
  }

 private:
  DemoControl* control_ = nullptr;
};

class DemoApp final : public px_application_event_handler {
 public:
  void open_files(const char* const* paths, int count) override {
    for (int i = 0; i < count; ++i) {
      std::println("[app] open_file {}", paths[i]);
    }
  }
  void new_file() override { std::println("[app] new_file"); }
  void appearance_changed() override {
    std::println("[app] appearance_changed dark={}", px_os_in_dark_mode() ? 1 : 0);
  }
};

}  // namespace

int main(int argc, char** argv) {
  // The event log is the useful output of this demo, and it is meant to be diffed against a trace
  // from another implementation. Block buffering would lose it on a kill and reorder it against
  // stderr, so opt out.
  //
  // This still governs on macOS, and on Windows whenever stdout is redirected: libc++'s
  // std::println routes through fwrite in both cases. Attached to a Windows console it instead
  // flushes and writes via WriteConsoleW, which is unbuffered anyway.
  std::setvbuf(stdout, nullptr, _IONBF, 0);

  px_init(kDemoAppName, "com.example.platform", argc, argv, 0);

  DemoApp app;
  px_set_application_event_handler(&app);

  DemoWindow win(900, 600, kDemoTitle, fcolor{0.09f, 0.10f, 0.12f, 1.0f});

  window_basic_aspect basic(&win);
  window_hover_aspect hover(&win);
  win.add_window_aspect(&basic);
  win.add_window_aspect(&hover);

  DemoControl root(&win, &hover);
  win.set_control(&root);
  win.set_root_control(&root);
  win.set_input_client(&root);

  win.show();

  std::println("platform: dark_mode={} caret_blink={:.3f}s scale={:.2f}",
               px_os_in_dark_mode() ? 1 : 0, px_caret_blink_time(), win.dpi_scale_factor());
  std::println("escape closes the window; drop files on it; type to exercise the IME path");

  px_run_event_loop();
  return 0;
}
