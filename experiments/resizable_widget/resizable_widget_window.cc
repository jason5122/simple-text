#include "resizable_widget_window.h"

#include "experiments/resizable_widget/resizable_widget_app.h"
#include "gui/renderer/renderer_lite.h"
#include "util/profile_util.h"

#include <fmt/base.h>

using namespace gui;

ResizableWidgetWindow::ResizableWidgetWindow(ResizableWidgetApp& parent,
                                             int width,
                                             int height,
                                             int wid)
    : Window(parent, width, height), parent(parent) {}

void ResizableWidgetWindow::onOpenGLActivate(const app::Size& size) {
    main_widget = std::make_shared<SolidColorWidget>(size, Rgba{255, 215, 0, 255});
}

void ResizableWidgetWindow::onDraw(const app::Size& size) {
    PROFILE_BLOCK("Total render time");

    main_widget->layout();
    main_widget->draw();

    gui::RendererLite::instance().flush(size);
}

void ResizableWidgetWindow::onResize(const app::Size& size) {
    main_widget->setSize(size);
}

void ResizableWidgetWindow::onScroll(const app::Point& mouse_pos, const app::Delta& delta) {
    main_widget->mousePositionChanged(mouse_pos);
    main_widget->scroll(mouse_pos, delta);
    redraw();
}
