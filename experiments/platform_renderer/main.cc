#include "gfx/frame.h"
#include "platform/app.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {

std::array<gfx::Quad, 8> make_animation_quads(double t, int viewport_width, int viewport_height) {
    constexpr float kPi = 3.14159265358979323846f;
    std::array<gfx::Quad, 8> quads{};

    const float cx = viewport_width * 0.5f;
    const float cy = viewport_height * 0.5f;
    const float radius = std::min(viewport_width, viewport_height) * 0.22f;
    const float size = 56.0f + 18.0f * std::sin(t * 2.0);

    for (size_t i = 0; i < 6; ++i) {
        const float phase = static_cast<float>(t * 1.8 + i * (kPi / 3.0f));
        const float x = cx + radius * std::cos(phase) - size * 0.5f;
        const float y = cy + radius * std::sin(phase) - size * 0.5f;
        const float hue = static_cast<float>(i) / 6.0f;
        quads[i] = gfx::Quad{
            x,
            y,
            size,
            size,
            0.25f + 0.75f * std::sin(hue * 6.28318f + 0.0f) * std::sin(hue * 6.28318f + 0.0f),
            0.25f + 0.75f * std::sin(hue * 6.28318f + 2.1f) * std::sin(hue * 6.28318f + 2.1f),
            0.25f + 0.75f * std::sin(hue * 6.28318f + 4.2f) * std::sin(hue * 6.28318f + 4.2f),
            1.0f};
    }

    const float bar_x = cx - 220.0f * std::cos(t * 2.4f);
    quads[6] = gfx::Quad{bar_x, cy - 260.0f, 18.0f, 520.0f, 0.05f, 0.05f, 0.05f, 1.0f};

    const float pulse = 120.0f + 40.0f * std::sin(t * 3.0f);
    quads[7] =
        gfx::Quad{cx - pulse * 0.5f, cy - pulse * 0.5f, pulse, pulse, 1.0f, 0.82f, 0.18f, 1.0f};

    return quads;
}

class RendererDelegate final : public platform::WindowDelegate {
public:
    void on_draw(platform::Window& window,
                 gfx::Frame& frame,
                 const platform::FrameInfo& frame_info) override {
        (void)window;
        frame.clear({1.0f, 1.0f, 1.0f, 1.0f});
        const auto quads =
            make_animation_quads(frame_info.time_seconds, frame_info.width_px, frame_info.height_px);
        frame.draw_quads(quads, 0, -scroll_y_);
    }

    void on_scroll(platform::Window& window, const platform::ScrollInfo& scroll_info) override {
        scroll_y_ += scroll_info.dy;
        window.request_redraw();
    }

private:
    float scroll_y_ = 0;
};

}  // namespace

int main() {
    std::setbuf(stdout, nullptr);

    auto app = platform::App::create({.renderer_backend = platform::RendererBackend::kOpenGL});
    if (!app) std::abort();

    RendererDelegate delegate;
    platform::Window* window = app->create_window(
        {.width = 1200, .height = 800, .title = "Platform Renderer"}, &delegate);
    if (!window) std::abort();

    window->set_continuous_redraw(true);
    return app->run();
}
