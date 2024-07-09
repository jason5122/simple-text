#include "renderer/renderer.h"
#include "status_bar_widget.h"

namespace gui {

StatusBarWidget::StatusBarWidget(const renderer::Size& size)
    : Widget{size}, line_column_label{new LabelWidget{{200, size.height}}} {
    line_column_label->setText("clangd, Line 1, Column 1");
}

void StatusBarWidget::draw() {
    renderer::RectRenderer& rect_renderer = renderer::Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position, size, {199, 203, 209, 255});

    line_column_label->draw();
}

void StatusBarWidget::layout() {
    line_column_label->setPosition(position + renderer::Point{32, 0});
}

}
