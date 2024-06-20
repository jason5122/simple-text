#include "status_bar_widget.h"

namespace gui {

StatusBarWidget::StatusBarWidget(std::shared_ptr<renderer::Renderer> renderer,
                                 const renderer::Size& size)
    : Widget{size}, renderer{renderer} {}

void StatusBarWidget::draw(const renderer::Size& screen_size, const renderer::Point& offset) {
    renderer->getRectRenderer().addRect(offset, {screen_size.width, size.height},
                                        {199, 203, 209, 255});
}

void StatusBarWidget::scroll(const renderer::Point& delta) {}

}
