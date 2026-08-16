#include "canvas/canvas.h"
#include "gfx/frame.h"
#include "gfx/texture.h"
#include "platform/app.h"
#include "text/font_rasterizer.h"
#include "text/types.h"
#include "ui/button.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
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
        const double phase = t * 1.8 + i * (kPi / 3.0);
        const float x = cx + radius * static_cast<float>(std::cos(phase)) - size * 0.5f;
        const float y = cy + radius * static_cast<float>(std::sin(phase)) - size * 0.5f;
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
        // Regenerate the background at native pixel resolution on first draw and on resize, so it
        // maps 1 texel : 1 pixel (sharp, no upscaling blur). Rebuilding the Canvas frees the old
        // texture (its GLTexture dtor runs here, with the GL context current).
        if (!canvas_ || cached_width_px_ != frame_info.width_px ||
            cached_height_px_ != frame_info.height_px) {
            canvas_ = std::make_unique<canvas::Canvas>(frame.device());
            cached_width_px_ = frame_info.width_px;
            cached_height_px_ = frame_info.height_px;
            background_ = make_checkerboard(*canvas_, cached_width_px_, cached_height_px_);
        }

        frame.clear({1.0f, 1.0f, 1.0f, 1.0f});

        // Background image, drawn 1:1 to fill the viewport.
        if (background_) {
            canvas_->draw_image(*background_, {0.0f, 0.0f, static_cast<float>(cached_width_px_),
                                               static_cast<float>(cached_height_px_)});
        }

        // Animated quads are recorded after the image, so they paint on top of it. Translucent so
        // the image shows through — confirms submission-order z-order across textured + solid draws.
        const auto quads = make_animation_quads(frame_info.time_seconds, frame_info.width_px,
                                                frame_info.height_px);
        for (const gfx::Quad& q : quads) {
            canvas_->fill_rect({q.x, q.y - scroll_y_, q.w, q.h}, {q.r, q.g, q.b, 0.85f});
        }

        canvas_->flush(frame);
    }

    void on_scroll(platform::Window& window, const platform::ScrollInfo& scroll_info) override {
        scroll_y_ += scroll_info.dy;
        window.request_redraw();
    }

private:
    static canvas::Image* make_checkerboard(canvas::Canvas& canvas, int width, int height) {
        if (width <= 0 || height <= 0) return nullptr;

        constexpr int kCell = 32;
        std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const bool on = ((x / kCell) + (y / kCell)) % 2 == 0;
                uint8_t* p = &pixels[(static_cast<size_t>(y) * width + x) * 4];
                p[0] = on ? 60 : 200;
                p[1] = on ? 90 : 200;
                p[2] = on ? 160 : 210;
                p[3] = 255;
            }
        }
        return canvas.create_image(width, height, gfx::TextureFormat::kRGBA8, pixels);
    }

    std::unique_ptr<canvas::Canvas> canvas_;
    canvas::Image* background_ = nullptr;
    int cached_width_px_ = 0;
    int cached_height_px_ = 0;
    float scroll_y_ = 0;
};

// Renders sample strings on a plain white background, for inspecting glyph rasterization.
class TextDelegate final : public platform::WindowDelegate {
public:
    void on_draw(platform::Window& window,
                 gfx::Frame& frame,
                 const platform::FrameInfo& frame_info) override {
        if (!canvas_) canvas_ = std::make_unique<canvas::Canvas>(frame.device());

        frame.clear({1.0f, 1.0f, 1.0f, 1.0f});

        if (!text_ready_) {
            auto& rasterizer = text::FontRasterizer::instance();
            constexpr std::string_view kPangrams[] = {
                "Sphinx of black quartz, judge my vow!",
                "The quick brown fox jumps over the lazy dog."};
            constexpr std::string_view kLoremIpsum[] = {
                "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod",
                "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam,",
                "quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo",
                "consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse",
                "cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non",
                "proident, sunt in culpa qui officia deserunt mollit anim id est laborum."};

            font_id_ = rasterizer.add_system_font(16);
            for (std::string_view str : kPangrams) {
                lines_.push_back(rasterizer.layout_line(font_id_, str));
            }

            small_font_id_ = rasterizer.add_system_font(16);
            for (std::string_view str : kLoremIpsum) {
                small_lines_.push_back(rasterizer.layout_line(small_font_id_, str));
            }
            text_ready_ = true;
        }

        auto& rasterizer = text::FontRasterizer::instance();
        const canvas::Color ink{51 / 255.f, 51 / 255.f, 51 / 255.f, 1.0f};

        const int line_height = rasterizer.metrics(font_id_).line_height;
        int y = 60 + 64;
        for (const text::LineLayout& line : lines_) {
            canvas_->draw_text(line, {0, y}, ink);
            y += line_height;
        }

        const int small_line_height = rasterizer.metrics(small_font_id_).line_height;
        y += line_height;  // gap between the two blocks
        for (const text::LineLayout& line : small_lines_) {
            canvas_->draw_text(line, {0, y}, ink);
            y += small_line_height;
        }

        canvas_->flush(frame);
    }

private:
    std::unique_ptr<canvas::Canvas> canvas_;
    text::FontId font_id_ = 0;
    text::FontId small_font_id_ = 0;
    std::vector<text::LineLayout> lines_;
    std::vector<text::LineLayout> small_lines_;
    bool text_ready_ = false;
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

    // for (int i = 0; i < 3; ++i) {
    //     auto delegate = std::make_unique<RendererDelegate>();
    //     platform::Window* window =
    //         app->create_window({.width = 900 + i * 120,
    //                             .height = 600 + i * 80,
    //                             .title = "Platform Renderer " + std::to_string(i + 1)},
    //                            delegate.get());
    //     if (!window) std::abort();

    //     window->set_continuous_redraw(true);
    //     delegates.push_back(std::move(delegate));
    // }

    auto text_delegate = std::make_unique<TextDelegate>();
    platform::Window* text_window = app->create_window(
        {.width = 800, .height = 400, .title = "Text"}, text_delegate.get());
    if (!text_window) std::abort();

    // platform::Window* coroutine_window =
    //     app->create_window({.width = 800, .height = 500, .title = "Coroutine Buttons"}, nullptr);
    // if (!coroutine_window) std::abort();

    // std::vector<std::unique_ptr<ui::Button>> buttons;
    // buttons.reserve(5);

    // auto fetch_button = std::make_unique<ui::Button>("Fetch data");
    // fetch_button->on_click_task([coroutine_window](ui::Button& button) -> corral::Task<void> {
    //     const int task_id = next_task_id();
    //     bool completed = false;
    //     std::println("task {} started", task_id);
    //     button.set_status_text("Loading...");
    //     co_await corral::try_([&]() -> corral::Task<void> {
    //         auto data = co_await fetch_data();
    //         auto result = process(std::move(data));
    //         co_await platform::resume_on_ui();
    //         coroutine_window->set_title(result);
    //         button.set_status_text("");
    //         completed = true;
    //     }).finally([task_id, &completed, &button]() -> corral::Task<void> {
    //         if (completed) {
    //             std::println("task {} completed", task_id);
    //             co_return;
    //         }

    //         std::println("task {} was cancelled", task_id);
    //         co_await platform::resume_on_ui();
    //         button.set_status_text("");
    //     });
    // });
    // fetch_button->add_to(*coroutine_window);
    // buttons.push_back(std::move(fetch_button));

    // add_title_button(buttons, *coroutine_window, "Set title: Alpha", "Alpha", 200ms);
    // add_title_button(buttons, *coroutine_window, "Set title: Beta", "Beta", 400ms);
    // add_title_button(buttons, *coroutine_window, "Set title: Gamma", "Gamma", 700ms);
    // add_title_button(buttons, *coroutine_window, "Set title: Delta", "Delta", 1100ms);

    return app->run();
}
