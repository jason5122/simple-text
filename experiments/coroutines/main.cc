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

}  // namespace

int main() {
    // Disable stdout buffering.
    std::setbuf(stdout, nullptr);

    auto application = app::create_app();

    auto window1 = application.create_window({.width = 1200, .height = 800});
    window1.set_title("Window 1");
    auto scope1 = window1.task_scope();

    auto button = app::Button::create("Fetch data");
    button.on_click([scope1, window1] {
        scope1.start([window1]() -> corral::Task<void> {
            auto data = co_await fetch_data();
            auto result = process(std::move(data));
            co_await app::resume_on_ui();
            window1.set_title(result);
        });
    });
    window1.add(std::move(button));

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
