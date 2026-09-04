// Canonical input-to-presentation benchmark for the platform layer.
//
// Mouse motion invalidates only the old and new follower bounds. On macOS this exercises the
// latency-critical CAOpenGLLayer path; the drawing code itself uses the same two-slot rectangle
// stream as the main demo. benchmark/record_platform_drag.sh drives and measures this binary.

#include <cstdio>
#include <cstdlib>

#include "experiments/platform/px/px.h"

namespace {

constexpr double kWindowWidth = 1400.0;
constexpr double kWindowHeight = 800.0;
constexpr double kSquareSize = 20.0;
constexpr fcolor kBackground{0.09f, 0.10f, 0.12f, 1.0f};
constexpr fcolor kOrange{0.95f, 0.55f, 0.25f, 1.0f};

rect square_rect(vec2 center) {
    return rect{center.x - kSquareSize * 0.5, center.y - kSquareSize * 0.5, kSquareSize,
                kSquareSize};
}

class DragBenchmarkHandler final : public px_window_event_handler {
public:
    void attach(px_window_t* window) { window_ = window; }

    bool handle_event(px_event_t* event) override {
        switch (event->type) {
        case PX_EVENT_MOUSE_MOTION:
            if (visible_) {
                px_mark_rect_dirty(window_, square_rect(position_));
            }
            position_ = event->pos;
            visible_ = true;
            px_mark_rect_dirty(window_, square_rect(position_));
            return true;
        case PX_EVENT_MOUSE_LEAVE:
            if (visible_) {
                px_mark_rect_dirty(window_, square_rect(position_));
                visible_ = false;
            }
            return true;
        case PX_EVENT_KEY:
            if (event->pressed && event->key == PX_KEY_ESCAPE) {
                px_close_window(window_);
                return true;
            }
            return false;
        default:
            return false;
        }
    }

    void paint(px_render_context* context,
               rect bounds,
               const rect* dirty,
               int dirty_count) override {
        context->begin_rect_batch();
        context->draw_rect(bounds, kBackground);

        if (visible_) {
            context->draw_rect(square_rect(position_), kOrange);
        }
        context->end_rect_batch();
    }

private:
    px_window_t* window_ = nullptr;
    vec2 position_;
    bool visible_ = false;
};

void print_geometry(px_window_t* window) {
    const vec2 origin = px_window_position(window);
    const vec2 size = px_window_size(window);
    const double x = origin.x + size.x * 0.5;
    const double y0 = origin.y + 50.0;
    const double y1 = origin.y + size.y - 50.0;
    std::printf("capture_rect=%.0f,%.0f,%.0f,%.0f\n", origin.x, origin.y, size.x, size.y);
    std::printf("high-speed mover: move_mouse %.0f %.0f %.0f %.0f 200 200 10 --drag\n", x, y0, x,
                y1);
}

}  // namespace

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    // The benchmark must be wholly event driven. Continuous animation is a different workload.
#if defined(_WIN32)
    _putenv_s("PX_NO_ANIMATION", "1");
#else
    setenv("PX_NO_ANIMATION", "1", 1);
#endif
    px_init("drag-benchmark", "com.example.drag-benchmark", argc, argv, 0);

    DragBenchmarkHandler handler;
    px_window_t* window = px_create_window(&handler, nullptr, kWindowWidth, kWindowHeight,
                                           "drag benchmark", kBackground, PX_WINDOW_DEFAULT);
    handler.attach(window);
    px_show_window(window);
    px_mark_dirty(window);
    print_geometry(window);

    px_run_event_loop();
    px_destroy_window(window);
    return 0;
}
