#include "experiments/gui_api_redesign/app.h"
#include <spdlog/spdlog.h>

int main() {
    auto app = App::create();

    auto* window = app->create_window(1200, 800);
    if (!window) {
        spdlog::error("oh dear");
        std::exit(1);
    }

    window->on_draw([](Renderer& renderer) {
        spdlog::info("draw");
        renderer.clear(0.5, 0, 0.5, 1);
    });
    return app->run();
}
