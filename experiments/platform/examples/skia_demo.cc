#include "experiments/platform/px/px.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>

namespace {

constexpr std::array<std::string_view, 5> kLines = {
    "Skia CPU renderer",
    "fx / CoreText — fi != -> 你好 مرحبا 👋",
    "Skia draws the geometry; fx still shapes and rasterizes every glyph.",
    "The pixels go directly from Skia's raster surface to Core Graphics.",
    "Resize, move the pointer, or press Escape to close.",
};

class SkiaDemo final : public px_window_event_handler {
public:
    void set_window(px_window_t* window) { window_ = window; }

    void set_fonts(px_font_t* heading, px_font_t* body) {
        heading_font_ = heading;
        body_font_ = body;
    }

    bool handle_event(px_event_t* event) override {
        if (event->type == PX_EVENT_KEY && event->pressed && event->key == PX_KEY_ESCAPE) {
            px_close_window(window_);
            return true;
        }
        if (event->type == PX_EVENT_MOUSE_MOTION) {
            const rect old_marker{pointer_.x - 12.0, pointer_.y - 12.0, 24.0, 24.0};
            pointer_ = event->pos;
            pointer_inside_ = true;
            px_mark_rect_dirty(window_, old_marker);
            px_mark_rect_dirty(window_, rect{pointer_.x - 12.0, pointer_.y - 12.0, 24.0, 24.0});
        } else if (event->type == PX_EVENT_MOUSE_LEAVE) {
            px_mark_rect_dirty(window_, rect{pointer_.x - 12.0, pointer_.y - 12.0, 24.0, 24.0});
            pointer_inside_ = false;
        }
        return false;
    }

    void paint(px_render_context* rc, rect bounds, const rect* dirty, int dirty_count) override {
        // Sublime passes the dirty union as `bounds`; it is not the window's geometry. A partial
        // cursor or animation paint must still lay out and redraw the same full-size scene.
        const vec2 size = px_window_size(window_);
        const rect window_bounds{0.0, 0.0, size.x, size.y};
        rc->draw_rect(window_bounds, fcolor{0.055f, 0.065f, 0.085f, 1.0f});

        constexpr double cell = 40.0;
        for (double y = 0.0; y < window_bounds.h; y += cell) {
            for (double x = 0.0; x < window_bounds.w; x += cell) {
                if ((static_cast<int>(x / cell) + static_cast<int>(y / cell)) % 2 == 0) {
                    rc->draw_rect(rect{x, y, cell, cell}, fcolor{0.07f, 0.08f, 0.105f, 1.0f});
                }
            }
        }

        const double panel_width = std::max(320.0, window_bounds.w - 96.0);
        rc->draw_rect(rect{48.0, 52.0, panel_width, 260.0}, fcolor{0.105f, 0.12f, 0.155f, 1.0f});
        rc->draw_rect(rect{48.0, 52.0, 6.0, 260.0}, fcolor{0.25f, 0.65f, 0.95f, 1.0f});

        if (heading_font_) {
            rc->draw_text(heading_font_, vec2{76.0, 96.0}, fcolor{0.86f, 0.92f, 1.0f, 1.0f},
                          kLines[0]);
        }
        if (body_font_) {
            for (size_t i = 1; i < kLines.size(); ++i) {
                rc->draw_text(body_font_, vec2{76.0, 104.0 + static_cast<double>(i) * 38.0},
                              i == 1 ? fcolor{0.70f, 0.83f, 0.97f, 1.0f}
                                     : fcolor{0.58f, 0.64f, 0.74f, 1.0f},
                              kLines[i]);
            }
        }

        rc->push_state(false);
        rc->restrict_clip_rect(rect{48.0, 340.0, panel_width, 120.0});
        rc->draw_rect(rect{48.0, 340.0, panel_width, 120.0}, fcolor{0.09f, 0.105f, 0.135f, 1.0f});
        const double track_width = std::max(1.0, panel_width - 120.0);
        const double animated_x = 58.0 + animation_position_ * track_width;
        rc->draw_rect(rect{animated_x, 370.0, 110.0, 60.0}, fcolor{0.94f, 0.44f, 0.22f, 1.0f});
        rc->draw_rect(rect{animated_x + 8.0, 378.0, 94.0, 44.0},
                      fcolor{0.12f, 0.14f, 0.18f, 1.0f});
        rc->pop_state();

        if (pointer_inside_) {
            rc->draw_rect(rect{pointer_.x - 10.0, pointer_.y - 10.0, 20.0, 20.0},
                          fcolor{0.98f, 0.76f, 0.25f, 1.0f});
        }
    }

    void animation_tick(double now) override {
        if (!window_) {
            return;
        }
        const vec2 size = px_window_size(window_);
        const double panel_width = std::max(320.0, size.x - 96.0);
        const double track_width = std::max(1.0, panel_width - 120.0);
        const double old_x = 58.0 + animation_position_ * track_width;
        animation_position_ = std::sin(now * 1.7) * 0.5 + 0.5;
        const double new_x = 58.0 + animation_position_ * track_width;
        const double left = std::min(old_x, new_x);
        const double right = std::max(old_x, new_x) + 110.0;
        px_mark_rect_dirty(window_, rect{left, 370.0, right - left, 60.0});
    }

private:
    px_window_t* window_ = nullptr;
    px_font_t* heading_font_ = nullptr;
    px_font_t* body_font_ = nullptr;
    vec2 pointer_;
    bool pointer_inside_ = false;
    double animation_position_ = 0.5;
};

}  // namespace

int main(int argc, char** argv) {
    px_init("skia_demo", "com.example.skia-demo", argc, argv, 0);

    SkiaDemo demo;
    px_window_t* window = px_create_window(
        &demo, nullptr, 900.0, 560.0, "skia_render_context demo",
        fcolor{0.055f, 0.065f, 0.085f, 1.0f}, PX_WINDOW_DEFAULT | PX_WINDOW_SOFTWARE);
    demo.set_window(window);
    demo.set_fonts(px_create_font("Menlo", 24.0f), px_create_font("Menlo", 14.0f));

    px_show_window(window);
    px_run_event_loop();
    px_destroy_window(window);
    return 0;
}
