#include "gui/renderer/renderer.h"
#include "status_bar_widget.h"

namespace gui {

StatusBarWidget::StatusBarWidget(const Size& size)
    : Widget{size}, line_column_label{new LabelWidget{{200, size.height}}} {
    line_column_label->setText("clangd, Line 1, Column 1", {64, 64, 64});
}

void StatusBarWidget::draw() {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();
    // rect_renderer.addRect(position, size, {199, 203, 209, 255});

    line_column_label->draw();
}

void StatusBarWidget::layout() {
    line_column_label->setPosition(position + kLeftPadding);
}

}
