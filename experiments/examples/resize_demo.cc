// A window for judging resize smoothness by eye.
//
// Written straight against px: one px_window_event_handler, no control tree, no animation, no
// environment tweaks. The scene is what a resize stresses -- a window full of shaped text plus
// edge markers -- and nothing else, so the backend is the only variable:
//
//   ./out/release/resize_demo          Metal, persistent texture + dirty rects + blit
//   PX_GL=1 ./out/release/resize_demo  GL, persistent FBO + dirty rects + blit (ST)
//
// The markers: a bar along the right and bottom edges and a square in the corner. During a drag
// they must stay glued to the window frame; any frame where the content lags the frame shows as
// the bars detaching. The size readout at the top-left changes every frame, so a repeated frame
// is visible as a number that fails to advance.

#include "fx/fx.h"
#include "px/px.h"
#include <cmath>
#include <cstdio>
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

class ResizeDemo final : public px_window_event_handler {
public:
    bool init() {
        font_ = px_create_font("Menlo", 12.0f);
        if (!font_) {
            std::fprintf(stderr, "resize_demo: cannot create font\n");
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

    bool handle_event(px_event_t* event) override {
        if (event->type == PX_EVENT_RESIZE) {
            size_ = event->size;
        }
        return false;
    }

    void paint(px_render_context* rc, rect bounds, const rect* dirty, int dirty_count) override {
        // `bounds` is the dirty union; the scene is laid out against the window.
        const rect window{0.0, 0.0, size_.x, size_.y};

        rc->begin_rect_batch();
        rc->draw_rect(window, fcolor{1.0f, 1.0f, 1.0f, 1.0f});
        rc->draw_rect(rect{window.w - kEdgeBar, 0.0, kEdgeBar, window.h},
                      fcolor{0.20f, 0.45f, 0.95f, 1.0f});
        rc->draw_rect(rect{0.0, window.h - kEdgeBar, window.w, kEdgeBar},
                      fcolor{0.20f, 0.45f, 0.95f, 1.0f});
        rc->draw_rect(rect{window.w - kCornerSquare, window.h - kCornerSquare, kCornerSquare,
                           kCornerSquare},
                      fcolor{0.95f, 0.30f, 0.25f, 1.0f});
        rc->end_rect_batch();

        // Fill the window with text, so every frame paints a scene proportional to the window
        // rather than a fixed strip. The last column and row are drawn even when only partly
        // visible, so shrinking cuts text off at the edge instead of dropping a whole column.
        const double line_height = std::max(1.0, static_cast<double>(metrics_.line_height));
        const double stride = column_width_ + kColumnGap;
        const int columns =
            std::max(1, static_cast<int>(std::ceil((window.w - 2.0 * kMargin) / stride)));
        const int rows =
            std::max(0, static_cast<int>(std::ceil((window.h - 2.0 * kMargin) / line_height)));
        const fcolor ink{0.15f, 0.16f, 0.18f, 1.0f};

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

        // Shaped every frame on purpose: it is tiny, and a stale number is the point.
        char readout[64];
        std::snprintf(readout, sizeof(readout), "%.0f x %.0f  frame %llu", size_.x, size_.y,
                      static_cast<unsigned long long>(++frame_));
        std::unique_ptr<fx_layout> readout_layout = px_shape_text(font_, readout);
        rc->begin_rect_batch();
        rc->draw_rect(rect{kMargin - 4.0, 2.0, readout_layout->advance + 8.0, line_height + 4.0},
                      fcolor{0.98f, 0.93f, 0.60f, 1.0f});
        rc->end_rect_batch();
        rc->draw_shaped_text(font_, vec2{kMargin, 4.0 + metrics_.ascent},
                             fcolor{0.0f, 0.0f, 0.0f, 1.0f}, readout_layout.get(), true);
    }

    void set_window(px_window_t* window) {
        window_ = window;
        size_ = px_window_size(window);
    }

private:
    px_window_t* window_ = nullptr;
    px_font_t* font_ = nullptr;
    px_font_metrics metrics_;
    std::vector<std::unique_ptr<fx_layout>> lines_;
    double column_width_ = 0.0;
    vec2 size_;
    unsigned long long frame_ = 0;
};

}  // namespace

int main(int argc, char** argv) {
    px_init("resize_demo", "com.jason5122.resize-demo", argc, argv, 0);

    ResizeDemo demo;
    if (!demo.init()) {
        return 1;
    }
    px_window_t* window = px_create_window(&demo, nullptr, 1200.0, 800.0, "resize demo",
                                           fcolor{1.0f, 1.0f, 1.0f, 1.0f}, PX_WINDOW_DEFAULT);
    demo.set_window(window);
    px_show_window(window);
    px_run_event_loop();
    px_destroy_window(window);
    return 0;
}
