// Resizes its own px window from the display clock and reports how the frames kept up. No mouse
// input is involved, so runs are repeatable and the numbers isolate px and the compositor from
// the drag itself. The measurement and the options live in resize_sweep.h.
//
//   ./out/release/resize_benchmark                 default backing (Metal on macOS)
//   PX_GL=1 ./out/release/resize_benchmark         ST's OpenGL layer
//   ./out/release/resize_benchmark --drag          measure a real drag instead: drag a corner
//                                                  yourself, then press Escape in the window
//   ./out/release/resize_benchmark --drag --dark   the same drag with a dark palette
//   ./out/release/resize_benchmark --drag --display-link   a drag with the display link running
//
// The scene is a window full of shaped text, as in resize_demo.

#include "benchmark/resize_sweep.h"
#include "fx/fx.h"
#include "px/px.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace {

constexpr const char* kLines[] = {
    "void gl_render_context::draw_rect(rect area, fill_mode fill) {",
    "    const rect transformed = transformed_rect(area);",
    "    const fcolor normalized = fill.color;",
    "    if (transformed.empty() || normalized.a <= 0.0f) return;",
    "    text_render_state().flush_pending_batch();",
    "    if (rect_batch_) {",
    "        rect_batch_->add(transformed, normalized);",
    "        return;",
    "    }",
    "    gl_rect_batch batch(device_size_);",
    "    batch.add(transformed, normalized);",
    "}",
    "",
    "// Glyph instances do not carry a clip rectangle. Submit child text while the",
    "// child scissor is still active, before restoring the parent state below.",
    "void gl_render_context::pop_state() {",
    "    if (state_stack_.empty()) return;",
    "    text_render_state().flush_pending_batch();",
    "    saved_state state = std::move(state_stack_.back());",
    "    state_stack_.pop_back();",
    "    translation_ = state.translation;",
    "    scale_ = state.scale;",
    "    clip_ = state.clip;",
    "    apply_clip();",
    "}",
};

constexpr double kMargin = 16.0;
constexpr double kColumnGap = 48.0;
constexpr double kEdgeBar = 6.0;
constexpr double kCornerSquare = 24.0;

std::string backend_label(bool dark) {
    const char* backend = "px platform default";
    if (std::getenv("PX_SKIA")) {
        backend = "px skia";
    }
#if defined(__APPLE__)
    else if (std::getenv("PX_GL")) {
        backend = "px opengl";
    } else {
        backend = "px metal";
    }
#endif
    return std::string(backend) + (dark ? " (dark)" : "");
}

struct Palette {
    fcolor background;
    fcolor ink;
};

constexpr Palette kLightPalette{{1.0f, 1.0f, 1.0f, 1.0f}, {0.15f, 0.16f, 0.18f, 1.0f}};
constexpr Palette kDarkPalette{{0.075f, 0.082f, 0.098f, 1.0f}, {0.78f, 0.80f, 0.86f, 1.0f}};

class ResizeBenchmark final : public px_window_event_handler {
public:
    explicit ResizeBenchmark(const resize_sweep::options& options)
        : drag_(options.drag),
          palette_(options.dark ? kDarkPalette : kLightPalette),
          recorder_(options, backend_label(options.dark)) {}

    fcolor background() const { return palette_.background; }

    bool init() {
        font_ = px_create_font("Menlo", 12.0f);
        if (!font_) {
            std::fprintf(stderr, "resize_benchmark: cannot create font\n");
            return false;
        }
        metrics_ = px_font_get_metrics(font_);
        for (const char* line : kLines) {
            std::unique_ptr<fx_layout> layout = px_shape_text(font_, line);
            column_width_ = std::max(column_width_, static_cast<double>(layout->advance));
            lines_.push_back(std::move(layout));
        }
        return true;
    }

    void set_window(px_window_t* window) {
        window_ = window;
        size_ = px_window_size(window);
    }

    bool handle_event(px_event_t* event) override {
        if (event->type == PX_EVENT_RESIZE) {
            size_ = event->size;
            if (drag_) {
                recorder_.resize_event(px_now(), current());
            }
        } else if (event->type == PX_EVENT_KEY && event->pressed && event->key == PX_KEY_ESCAPE) {
            recorder_.finish(px_now());
            px_exit_event_loop();
            return true;
        }
        return false;
    }

    void animation_tick(double) override {
        resize_sweep::size requested;
        if (recorder_.tick(px_now(), current(), &requested)) {
            px_set_window_size(window_, requested.w, requested.h);
            recorder_.set_size_done(px_now());
        }
        if (recorder_.finished()) {
            px_exit_event_loop();
        }
    }

    void paint(px_render_context* rc, rect bounds, const rect* dirty, int dirty_count) override {
        recorder_.paint_begin(px_now(), current());
        draw_scene(rc);
        recorder_.paint_end(px_now());
    }

private:
    resize_sweep::size current() const { return resize_sweep::size{size_.x, size_.y}; }

    void draw_scene(px_render_context* rc) {
        const rect window{0.0, 0.0, size_.x, size_.y};

        rc->begin_rect_batch();
        rc->draw_rect(window, palette_.background);
        rc->draw_rect(rect{window.w - kEdgeBar, 0.0, kEdgeBar, window.h},
                      fcolor{0.20f, 0.45f, 0.95f, 1.0f});
        rc->draw_rect(rect{0.0, window.h - kEdgeBar, window.w, kEdgeBar},
                      fcolor{0.20f, 0.45f, 0.95f, 1.0f});
        rc->draw_rect(rect{window.w - kCornerSquare, window.h - kCornerSquare, kCornerSquare,
                           kCornerSquare},
                      fcolor{0.95f, 0.30f, 0.25f, 1.0f});
        rc->end_rect_batch();

        const double line_height = std::max(1.0, static_cast<double>(metrics_.line_height));
        const double stride = column_width_ + kColumnGap;
        const int columns =
            std::max(1, static_cast<int>(std::ceil((window.w - 2.0 * kMargin) / stride)));
        const int rows =
            std::max(0, static_cast<int>(std::ceil((window.h - 2.0 * kMargin) / line_height)));
        const fcolor ink = palette_.ink;

        rc->begin_text_batch();
        for (int column = 0; column < columns; ++column) {
            const double x = kMargin + column * stride;
            for (int row = 0; row < rows; ++row) {
                fx_layout* layout = lines_[static_cast<size_t>(row) % lines_.size()].get();
                if (layout->glyphs.empty()) continue;
                const double baseline = kMargin + row * line_height + metrics_.ascent;
                rc->draw_shaped_text(font_, vec2{x, baseline}, ink, layout, true);
            }
        }
        rc->end_text_batch();
    }

    bool drag_ = false;
    Palette palette_;
    resize_sweep::recorder recorder_;
    px_window_t* window_ = nullptr;
    px_font_t* font_ = nullptr;
    px_font_metrics metrics_;
    std::vector<std::unique_ptr<fx_layout>> lines_;
    double column_width_ = 0.0;
    vec2 size_;
};

}  // namespace

int main(int argc, char** argv) {
    resize_sweep::options options;
    if (!resize_sweep::parse_options(argc, argv, &options)) {
        resize_sweep::usage(argv[0]);
        return 2;
    }

    px_init("resize_benchmark", "com.jason5122.resize-benchmark", argc, argv, 0);

    ResizeBenchmark benchmark(options);
    if (!benchmark.init()) {
        return 1;
    }
    const resize_sweep::size initial =
        options.drag ? resize_sweep::size{1200.0, 800.0} : options.maximum;
    px_window_t* window =
        px_create_window(&benchmark, nullptr, initial.w, initial.h, "resize benchmark",
                         benchmark.background(), PX_WINDOW_DEFAULT);
    benchmark.set_window(window);
    // Anchored near the top-left so the whole sweep stays on screen.
    px_set_window_position(window, vec2{20.0, 40.0});
    if (options.drag) {
        std::printf("drag a corner or edge of the window, then press Escape in it\n");
        std::fflush(stdout);
    }
    px_show_window(window);
    // The sweep is clocked by the display link. A drag is event driven, like any px window, unless
    // asked to keep the link running for comparison.
    if (!options.drag || options.display_link) {
        px_set_animating(window, true);
    }
    px_run_event_loop();
    px_destroy_window(window);
    return 0;
}
