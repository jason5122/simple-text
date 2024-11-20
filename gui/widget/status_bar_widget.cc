#include "gui/renderer/renderer.h"
#include "status_bar_widget.h"

namespace gui {

StatusBarWidget::StatusBarWidget(const app::Size& size, size_t font_id)
    : Widget{size}, line_column_label{new LabelWidget{font_id, {0, size.height}}} {
    line_column_label->setText("clangd, Line 1, Column 1", kTextColor);
}

void StatusBarWidget::draw() {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position, size, kStatusBarColor, RectRenderer::RectLayer::kForeground);

    line_column_label->draw();
}

void StatusBarWidget::layout() {
    line_column_label->setSize(size);
    line_column_label->setPosition(position + kLeftPadding);
}

}  // namespace gui
