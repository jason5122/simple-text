#include "gfx/frame.h"
#include "platform/app.h"
#include "ui/button.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <print>
#include <string>
#include <vector>

using namespace std::chrono_literals;

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
        frame.clear({1.0f, 1.0f, 1.0f, 1.0f});
        const auto quads = make_animation_quads(frame_info.time_seconds, frame_info.width_px,
                                                frame_info.height_px);
        frame.draw_quads(quads, 0, -scroll_y_);
    }

    void on_scroll(platform::Window& window, const platform::ScrollInfo& scroll_info) override {
        scroll_y_ += scroll_info.dy;
        window.request_redraw();
    }

private:
    float scroll_y_ = 0;
};

corral::Task<std::string> fetch_data() {
    co_await platform::sleep_for(1s);
    co_return "Fetched data";
}

int next_task_id() {
    static int next_id = 1;
    return next_id++;
}

std::string process(std::string data) { return std::move(data) + " processed"; }

void add_title_button(std::vector<std::unique_ptr<ui::Button>>& buttons,
                      platform::Window& window,
                      std::string button_title,
                      std::string next_title,
                      std::chrono::milliseconds delay) {
    auto button = std::make_unique<ui::Button>(std::move(button_title));
    button->on_click_task([&window, next_title = std::move(next_title),
                           delay](ui::Button& button) -> corral::Task<void> {
        const int task_id = next_task_id();
        bool completed = false;
        std::println("task {} started", task_id);
        button.set_status_text("Loading...");
        co_await corral::try_([&]() -> corral::Task<void> {
            co_await platform::sleep_for(delay);
            co_await platform::resume_on_ui();
            window.set_title(next_title);
            button.set_status_text("");
            completed = true;
        }).finally([task_id, &completed, &button]() -> corral::Task<void> {
            if (completed) {
                std::println("task {} completed", task_id);
                co_return;
            }

            std::println("task {} was cancelled", task_id);
            co_await platform::resume_on_ui();
            button.set_status_text("");
        });
    });
    button->add_to(window);
    buttons.push_back(std::move(button));
}

}  // namespace

int main() {
    std::setbuf(stdout, nullptr);

    auto app = platform::App::create({.renderer_backend = platform::RendererBackend::kOpenGL});
    if (!app) std::abort();

    std::vector<std::unique_ptr<RendererDelegate>> delegates;
    delegates.reserve(3);

    for (int i = 0; i < 3; ++i) {
        auto delegate = std::make_unique<RendererDelegate>();
        platform::Window* window =
            app->create_window({.width = 900 + i * 120,
                                .height = 600 + i * 80,
                                .title = "Platform Renderer " + std::to_string(i + 1)},
                               delegate.get());
        if (!window) std::abort();

        window->set_continuous_redraw(true);
        delegates.push_back(std::move(delegate));
    }

    platform::Window* coroutine_window =
        app->create_window({.width = 800, .height = 500, .title = "Coroutine Buttons"}, nullptr);
    if (!coroutine_window) std::abort();

    std::vector<std::unique_ptr<ui::Button>> buttons;
    buttons.reserve(5);

    auto fetch_button = std::make_unique<ui::Button>("Fetch data");
    fetch_button->on_click_task([coroutine_window](ui::Button& button) -> corral::Task<void> {
        const int task_id = next_task_id();
        bool completed = false;
        std::println("task {} started", task_id);
        button.set_status_text("Loading...");
        co_await corral::try_([&]() -> corral::Task<void> {
            auto data = co_await fetch_data();
            auto result = process(std::move(data));
            co_await platform::resume_on_ui();
            coroutine_window->set_title(result);
            button.set_status_text("");
            completed = true;
        }).finally([task_id, &completed, &button]() -> corral::Task<void> {
            if (completed) {
                std::println("task {} completed", task_id);
                co_return;
            }

            std::println("task {} was cancelled", task_id);
            co_await platform::resume_on_ui();
            button.set_status_text("");
        });
    });
    fetch_button->add_to(*coroutine_window);
    buttons.push_back(std::move(fetch_button));

    add_title_button(buttons, *coroutine_window, "Set title: Alpha", "Alpha", 200ms);
    add_title_button(buttons, *coroutine_window, "Set title: Beta", "Beta", 400ms);
    add_title_button(buttons, *coroutine_window, "Set title: Gamma", "Gamma", 700ms);
    add_title_button(buttons, *coroutine_window, "Set title: Delta", "Delta", 1100ms);

    return app->run();
}
