#include "experiments/coroutines/app.h"

#include <print>
#include <string>
#include <utility>

using namespace std::chrono_literals;

namespace {

corral::Task<std::string> fetch_data() {
    co_await app::sleep_for(1s);
    co_return "Fetched data";
}

std::string process(std::string data) { return std::move(data) + " processed"; }

void add_title_button(const app::Window& window,
                      const app::TaskScope& scope,
                      std::string button_title,
                      std::string next_title,
                      std::chrono::milliseconds delay) {
    auto button = app::Button::create(std::move(button_title));
    button.on_click([scope, window, next_title = std::move(next_title), delay] {
        scope.start([window, next_title, delay]() -> corral::Task<void> {
            co_await app::sleep_for(delay);
            co_await app::resume_on_ui();
            window.set_title(next_title);
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
    auto scope1 = window1.task_scope();

    auto fetch_button = app::Button::create("Fetch data");
    fetch_button.on_click([scope1, window1] {
        scope1.start([window1]() -> corral::Task<void> {
            auto data = co_await fetch_data();
            auto result = process(std::move(data));
            co_await app::resume_on_ui();
            window1.set_title(result);
        });
    });
    window1.add(std::move(fetch_button));

    add_title_button(window1, scope1, "Set title: Alpha", "Alpha", 200ms);
    add_title_button(window1, scope1, "Set title: Beta", "Beta", 400ms);
    add_title_button(window1, scope1, "Set title: Gamma", "Gamma", 700ms);
    add_title_button(window1, scope1, "Set title: Delta", "Delta", 1100ms);

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
