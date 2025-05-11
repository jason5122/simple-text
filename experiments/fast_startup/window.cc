#include "window.h"

#include "experiments/fast_startup/app.h"
#include "gui/renderer/debug/renderer_lite.h"

#include <fmt/base.h>

namespace gui {

FastStartupWindow::FastStartupWindow(FastStartupApp& parent, int width, int height, int wid)
    : WindowWidget(parent, width, height) {}

void FastStartupWindow::draw() { RendererLite::instance().flush(size(), {255, 0, 0}); }

void FastStartupWindow::layout() { redraw(); }

}  // namespace gui
