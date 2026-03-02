#include "app/app.h"
#include <cstdlib>

int main() {
    // Disable stdout buffering.
    std::setbuf(stdout, nullptr);

    auto app = app::create_app(app::Backend::kOpenGL);
    if (!app) std::abort();

    auto window1 = app->create_window({.width = 1200, .height = 800});
    window1->set_title("Window 1");

    auto window2 = app->create_window({.width = 1200, .height = 800});
    window2->set_title("Window 2");

    app->run();
}
