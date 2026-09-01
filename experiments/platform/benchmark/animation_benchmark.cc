#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "experiments/platform/px/px.h"

namespace {

constexpr fcolor kBackground{0.06f, 0.07f, 0.08f, 1.0f};
constexpr fcolor kBarColor{0.20f, 0.90f, 0.45f, 1.0f};
constexpr double kBarWidth = 24.0;
constexpr double kBarHeight = 96.0;
constexpr double kBarSpeed = 120.0;
constexpr double kBarStart = 32.0;

class AnimationHandler final : public px_window_event_handler {
public:
    explicit AnimationHandler(int rectangle_count) : rectangle_count_(rectangle_count) {}

    void attach(px_window_t* window) { window_ = window; }

    bool handle_event(px_event_t* event) override {
        if (event->type == PX_EVENT_SCROLL) {
            ++scroll_event_count_;
            scroll_distance_ += std::abs(event->scroll_delta.y);
            if (!reported_scroll_input_ && scroll_distance_ >= 640.0 * 12.0) {
                reported_scroll_input_ = true;
                std::printf("scroll_input_complete events=%d distance=%.0f\n", scroll_event_count_,
                            scroll_distance_);
            }
            px_mark_dirty(window_);
            return true;
        }
        if (event->type == PX_EVENT_KEY && event->pressed && event->key == PX_KEY_ESCAPE) {
            px_close_window(window_);
            return true;
        }
        return false;
    }

    void animation_tick(double now) override {
        if (start_time_ == 0.0) {
            start_time_ = now;
        }
        bar_x_ = kBarStart + (now - start_time_) * kBarSpeed;
        px_mark_dirty(window_);
    }

    void paint(px_render_context* context,
               rect bounds,
               const rect* dirty,
               int dirty_count) override {
        (void)dirty;
        (void)dirty_count;
        context->begin_rect_batch();
        context->draw_rect(bounds, kBackground);
        for (int i = 0; i < rectangle_count_; ++i) {
            const double x = std::fmod(static_cast<double>(i * 37), std::max(1.0, bounds.w - 24.0));
            const double y = std::fmod(static_cast<double>(i * 53), std::max(1.0, bounds.h - 24.0));
            context->draw_rect(rect{x, y, 24.0, 24.0}, kBackground);
        }
        context->draw_rect(rect{bar_x_, (bounds.h - kBarHeight) * 0.5, kBarWidth, kBarHeight},
                           kBarColor);
        context->end_rect_batch();
    }

private:
    px_window_t* window_ = nullptr;
    double start_time_ = 0.0;
    double bar_x_ = kBarStart;
    int rectangle_count_ = 0;
    int scroll_event_count_ = 0;
    double scroll_distance_ = 0.0;
    bool reported_scroll_input_ = false;
};

void usage(const char* program) {
    std::fprintf(stderr, "usage: %s small|large [rectangle_count]\n", program);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3 ||
        (std::strcmp(argv[1], "small") != 0 && std::strcmp(argv[1], "large") != 0)) {
        usage(argv[0]);
        return 2;
    }
    char* end = nullptr;
    const long requested_rectangles = argc == 3 ? std::strtol(argv[2], &end, 10) : 0;
    if (requested_rectangles < 0 || requested_rectangles > 100'000 ||
        (argc == 3 && (!end || *end != '\0'))) {
        usage(argv[0]);
        return 2;
    }
    const int rectangle_count = static_cast<int>(requested_rectangles);
    const bool large = std::strcmp(argv[1], "large") == 0;
    const double width = large ? 1400.0 : 900.0;
    const double height = large ? 800.0 : 600.0;

    std::setvbuf(stdout, nullptr, _IONBF, 0);
    px_init("animation-benchmark", "com.example.animation-benchmark", argc, argv, 0);

    AnimationHandler handler(rectangle_count);
    px_window_t* window = px_create_window(&handler, nullptr, width, height, "animation benchmark",
                                           kBackground, PX_WINDOW_DEFAULT);
    handler.attach(window);
    px_show_window(window);
    px_mark_dirty(window);

    const vec2 origin = px_window_position(window);
    const vec2 size = px_window_size(window);
    std::printf("capture_rect=%.0f,%.0f,%.0f,%.0f\n", origin.x, origin.y, size.x, size.y);
    std::printf("bar_speed=%.0f points/s\n", kBarSpeed);
    std::printf("rectangle_count=%d\n", rectangle_count);

    px_run_event_loop();
    px_destroy_window(window);
    return 0;
}
