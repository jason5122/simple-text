#include "experiments/coroutines/app.h"

#include <atomic>
#include <print>
#include <string>
#include <utility>

using namespace std::chrono_literals;

namespace {

corral::Task<std::string> fetch_data() {
    co_await app::sleep_for(1s);
    co_return "Fetched data";
}

int next_task_id() {
    static std::atomic<int> next_id = 1;
    return next_id.fetch_add(1, std::memory_order_relaxed);
}

std::string process(std::string data) { return std::move(data) + " processed"; }

void add_title_button(const app::Window& window,
                      std::string button_title,
                      std::string next_title,
                      app::Color next_color,
                      std::chrono::milliseconds delay) {
    auto button = app::Button::create(std::move(button_title));
    button.on_click_task([window, next_title = std::move(next_title), next_color,
                          delay](app::Button button) -> corral::Task<void> {
        const int task_id = next_task_id();
        bool completed = false;
        std::println("task {} started", task_id);
        button.set_status_text("Loading...");
        co_await corral::try_([&]() -> corral::Task<void> {
            co_await app::sleep_for(delay);
            co_await app::resume_on_ui();
            window.set_background_color(next_color);
            window.set_title(next_title);
            button.set_status_text("");
            completed = true;
        }).finally([task_id, &completed, button]() -> corral::Task<void> {
            if (completed) {
                std::println("task {} completed", task_id);
                co_return;
            }

            std::println("task {} was cancelled", task_id);
            co_await app::resume_on_ui();
            button.set_status_text("");
        });
    });
    window.add(std::move(button));
}

}  // namespace

int main() {
    // Disable stdout buffering.
    std::setbuf(stdout, nullptr);

    auto application = app::create_app();

    auto window1 = application.create_window({.width = 1200, .height = 800});
    window1.set_title("Window 1");

    auto fetch_button = app::Button::create("Fetch data");
    fetch_button.on_click_task([window1](app::Button button) -> corral::Task<void> {
        const int task_id = next_task_id();
        bool completed = false;
        std::println("task {} started", task_id);
        button.set_status_text("Loading...");
        co_await corral::try_([&]() -> corral::Task<void> {
            auto data = co_await fetch_data();
            auto result = process(std::move(data));
            co_await app::resume_on_ui();
            window1.set_background_color({.red = 0.80, .green = 0.92, .blue = 0.84});
            window1.set_title(result);
            button.set_status_text("");
            completed = true;
        }).finally([task_id, &completed, button]() -> corral::Task<void> {
            if (completed) {
                std::println("task {} completed", task_id);
                co_return;
            }

            std::println("task {} was cancelled", task_id);
            co_await app::resume_on_ui();
            button.set_status_text("");
        });
    });
    window1.add(std::move(fetch_button));

    add_title_button(window1, "Set title: Alpha", "Alpha",
                     {.red = 0.96, .green = 0.82, .blue = 0.82}, 200ms);
    add_title_button(window1, "Set title: Beta", "Beta",
                     {.red = 0.82, .green = 0.88, .blue = 0.98}, 400ms);
    add_title_button(window1, "Set title: Gamma", "Gamma",
                     {.red = 0.86, .green = 0.97, .blue = 0.82}, 700ms);
    add_title_button(window1, "Set title: Delta", "Delta",
                     {.red = 0.98, .green = 0.91, .blue = 0.78}, 1100ms);

    auto window2 = application.create_window({.width = 800, .height = 500});
    window2.set_title("Window 2");
    auto scope2 = window2.task_scope();
    window2.on_close_request([scope2] {
        std::println("closing window 2");
        scope2.cancel();
        return true;
    });

    application.run();
}
