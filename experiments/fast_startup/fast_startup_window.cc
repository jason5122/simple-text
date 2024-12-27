#include "fast_startup_window.h"

#include "experiments/fast_startup/fast_startup_app.h"
#include "gui/renderer/renderer_lite.h"

#include <fmt/base.h>

FastStartupWindow::FastStartupWindow(FastStartupApp& parent, int width, int height, int wid)
    : Window(parent, width, height), parent(parent) {
    main_widget = std::make_unique<gui::CustomWidget>("hello world!", parent.main_font_id);
}

void FastStartupWindow::onOpenGLActivate(const app::Size& size) {
    main_widget->setSize(size);
}

void FastStartupWindow::onDraw(const app::Size& size) {
    main_widget->layout();
    main_widget->draw();

    gui::RendererLite::instance().flush(size);
}

void FastStartupWindow::onResize(const app::Size& size) {
    redraw();
}

void FastStartupWindow::onScroll(const app::Point& mouse_pos, const app::Delta& delta) {
    main_widget->mousePositionChanged(mouse_pos);
    main_widget->scroll(mouse_pos, delta);
    redraw();
}
