#include "px/px.h"
#include <array>
#include <string_view>

namespace {

constexpr double kWindowWidth = 960.0;
constexpr double kWindowHeight = 620.0;
constexpr fcolor kBackground{1.0f, 1.0f, 1.0f, 1.0f};

struct LineSpec {
    std::string_view text;
    float size;
    double baseline;
    fcolor color;
};

constexpr std::array<LineSpec, 4> kLineSpecs = {
    LineSpec{"Coral glow - 16 pt", 16.0f, 90.0, {0.92f, 0.18f, 0.15f, 1.0f}},
    LineSpec{"Ocean glow - 24 pt", 24.0f, 180.0, {0.04f, 0.38f, 0.92f, 1.0f}},
    LineSpec{"Violet glow - 36 pt", 36.0f, 310.0, {0.52f, 0.16f, 0.84f, 1.0f}},
    LineSpec{"Emerald glow - 52 pt", 52.0f, 475.0, {0.0f, 0.52f, 0.29f, 1.0f}},
};

void draw_glowing_text(px_render_context* context, const LineSpec& line) {
    px_font_t* font = px_create_font("Source Code Pro", line.size, PX_FONT_GLOW);
    if (!font) {
        return;
    }
    context->draw_text(font, {64.0, line.baseline}, line.color, line.text);
}

class GlowDemo final : public px_window_event_handler {
public:
    void attach(px_window_t* window) { window_ = window; }

    bool handle_event(px_event_t* event) override {
        if (event->type == PX_EVENT_KEY && event->pressed && event->key == PX_KEY_ESCAPE) {
            px_close_window(window_);
            return true;
        }
        return false;
    }

    void paint(px_render_context* context,
               rect bounds,
               const rect* dirty,
               int dirty_count) override {
        context->draw_rect(bounds, kBackground);
        for (const LineSpec& line : kLineSpecs) {
            draw_glowing_text(context, line);
        }
    }

private:
    px_window_t* window_ = nullptr;
};

}  // namespace

int main(int argc, char** argv) {
    px_init("glow-demo", "com.example.glow-demo", argc, argv, 0);

    GlowDemo demo;
    px_window_t* window = px_create_window(&demo, nullptr, kWindowWidth, kWindowHeight,
                                           "fx font glow demo", kBackground, PX_WINDOW_DEFAULT);
    if (!window) {
        return 1;
    }
    demo.attach(window);
    px_show_window(window);
    px_mark_dirty(window);
    px_run_event_loop();
    px_destroy_window(window);
    return 0;
}
