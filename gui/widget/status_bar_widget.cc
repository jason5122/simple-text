#include "status_bar_widget.h"

#include "gui/renderer/renderer.h"

namespace gui {

StatusBarWidget::StatusBarWidget(int min_height, size_t font_id)
    : HorizontalLayoutWidget(0, 32, 0),
      line_column_label(std::make_unique<LabelWidget>(font_id, kTextColor)) {

    int label_height = line_column_label->getHeight();
    setHeight(std::max(label_height, min_height));

    addChildStart(line_column_label);
}

void StatusBarWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position, size, kStatusBarColor, Layer::kTwo);

    HorizontalLayoutWidget::draw();
}

void StatusBarWidget::setText(std::string_view str8) {
    line_column_label->setText(str8);
}

}  // namespace gui
