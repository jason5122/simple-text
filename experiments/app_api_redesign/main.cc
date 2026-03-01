#include "app/app.h"
#include <cstdlib>

int main() {
    auto app = app::create_app(app::Backend::kOpenGL);
    if (!app) std::abort();

    auto window = app->create_window({
        .width = 1200,
        .height = 800,
        .title = "Bare macOS App",
    });
    window->set_title("Hello");

    app->run();
}
