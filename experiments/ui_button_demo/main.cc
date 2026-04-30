#include "platform/app.h"
#include "ui/button.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <print>
#include <memory>

using namespace std::chrono_literals;

namespace {

int next_task_id() {
    static std::atomic<int> next_id = 1;
    return next_id.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace

int main() {
    std::setbuf(stdout, nullptr);

    auto app = platform::App::create({.renderer_backend = platform::RendererBackend::kOpenGL});
    if (!app) std::abort();

    platform::Window* window =
        app->create_window({.width = 640, .height = 420, .title = "UI Button Demo"}, nullptr);
    if (!window) std::abort();

    ui::Button fetch_button("Fetch data");
    fetch_button.on_click_task([window](ui::Button& button) -> corral::Task<void> {
        const int task_id = next_task_id();
        bool completed = false;
        std::println("task {} started", task_id);
        button.set_status_text("Loading...");
        co_await corral::try_([&]() -> corral::Task<void> {
            co_await platform::sleep_for(1s);
            co_await platform::resume_on_ui();
            window->set_title("Fetched data");
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
    fetch_button.add_to(*window);

    ui::Button quick_button("Set title after 250ms");
    quick_button.on_click_task([window](ui::Button& button) -> corral::Task<void> {
        button.set_status_text("Waiting...");
        co_await platform::sleep_for(250ms);
        co_await platform::resume_on_ui();
        window->set_title("Coroutine button clicked");
        button.set_status_text("");
    });
    quick_button.add_to(*window);

    return app->run();
}
