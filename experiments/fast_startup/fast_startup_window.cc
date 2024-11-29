#include "fast_startup_window.h"

#include "experiments/fast_startup/fast_startup_app.h"

#include "opengl/gl.h"
using namespace opengl;

FastStartupWindow::FastStartupWindow(FastStartupApp& parent, int width, int height, int wid)
    : Window{parent, width, height} {}

void FastStartupWindow::onDraw(const app::Size& size) {
    glViewport(0, 0, size.width, size.height);
    glClear(GL_COLOR_BUFFER_BIT);

    glClearColor(253.0f / 255, 253.0f / 255, 253.0f / 255, 1.0f);  // Light.
    // glClearColor(48.0f / 255, 56.0f / 255, 65.0f / 255, 1.0f);  // Dark.
}
