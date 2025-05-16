#include "experiments/gui_api_redesign/app.h"
#include <fmt/base.h>

int main() {
    // Disable stdout buffering.
    std::setbuf(stdout, nullptr);

    App app;
    if (!app.initialize()) std::exit(1);

    Window& win = app.create_window(1200, 800);
    win.on_draw([]() { fmt::println("draw"); });

    return app.run();
}
