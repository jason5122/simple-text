#include "experiments/gui_api_redesign/app.h"
#include <fmt/base.h>

int main() {
    // Disable stdout buffering.
    std::setbuf(stdout, nullptr);

    App app;

    Window& w1 = app.create_window(800, 600);
    Window& w2 = app.create_window(800, 600);
    w1.on_draw([]() { fmt::println("window 1 draw"); });
    w2.on_draw([]() { fmt::println("window 2 draw"); });

    return app.run();
}
