#include "experiments/gui_api_redesign/app.h"
#include <fmt/base.h>

// TODO: Remove this.
#include "experiments/opengl_loader_redesign/gl.h"
using namespace opengl_redesign;

int main() {
    // Disable stdout buffering.
    std::setbuf(stdout, nullptr);

    auto app = App::create();

    Window& win = app->create_window(1200, 800);
    win.on_draw([]() {
        fmt::println("draw");
        glClearColor(0.5, 0, 0.5, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    });

    return app->run();
}
