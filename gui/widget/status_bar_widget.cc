#include "gui/renderer/renderer.h"
#include "status_bar_widget.h"

namespace gui {

StatusBarWidget::StatusBarWidget(const Size& size)
    : Widget{size}, line_column_label{new LabelWidget{{0, size.height}}} {
    line_column_label->setText("clangd, Line 1, Column 1", {64, 64, 64});
}

void StatusBarWidget::draw(const std::optional<Point>& mouse_pos) {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position, size, kStatusBarColor, RectRenderer::RectLayer::kForeground);

    line_column_label->draw(mouse_pos);
}

void StatusBarWidget::layout() {
    line_column_label->setSize(size);
    line_column_label->setPosition(position + kLeftPadding);
}

}
