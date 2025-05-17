#include "experiments/gui_api_redesign/app.h"
#include <fmt/base.h>

// TODO: Remove this.
#include "opengl/gl.h"
using namespace opengl;

int main() {
    // Disable stdout buffering.
    std::setbuf(stdout, nullptr);

    auto app = App::create();

    auto& window = app->create_window(1200, 800);
    window.on_draw([]() {
        fmt::println("draw");
        glClearColor(0.5, 0, 0.5, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        // TODO: Move this to Renderer::end_frame().
        glFlush();
    });

    fmt::println("hello world");

    return app->run();
}
