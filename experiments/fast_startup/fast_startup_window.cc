#include "fast_startup_window.h"

#include "experiments/fast_startup/fast_startup_app.h"
#include "gui/renderer/renderer.h"

#include "util/std_print.h"

FastStartupWindow::FastStartupWindow(FastStartupApp& parent, int width, int height, int wid)
    : Window{parent, width, height} {}

void FastStartupWindow::onDraw(const app::Size& size) {
    auto& rect_renderer = gui::Renderer::instance().getRectRenderer();
    rect_renderer.addRect({0, 0}, {100, 100}, {255, 0, 0},
                          gui::RectRenderer::RectLayer::kForeground);

    auto& image_renderer = gui::Renderer::instance().getImageRenderer();
    app::Point coords = {size.width - 1426, 0};
    gui::Rgba color = {255, 255, 255, true};
    image_renderer.addImage(gui::ImageRenderer::kStanfordBunny, coords, color);

    gui::Renderer::instance().flush(size);
}

void FastStartupWindow::onResize(const app::Size& size) {
    redraw();
}
