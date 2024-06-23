#include "renderer/renderer.h"
#include "status_bar_widget.h"

namespace gui {

StatusBarWidget::StatusBarWidget(const renderer::Size& size) : Widget{size} {}

void StatusBarWidget::draw(const renderer::Size& screen_size) {
    renderer::RectRenderer& rect_renderer = renderer::g_renderer->getRectRenderer();
    rect_renderer.addRect(position, size, {199, 203, 209, 255});
}

}
