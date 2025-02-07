#include "status_bar_widget.h"

#include "gui/renderer/renderer.h"

namespace gui {

StatusBarWidget::StatusBarWidget(int min_height, size_t font_id)
    : HorizontalLayoutWidget(0, 32, 0), line_column_label(new LabelWidget(font_id, kTextColor)) {

    int label_height = line_column_label->height();
    set_height(std::max(label_height, min_height));

    addChildStart(std::unique_ptr<LabelWidget>(line_column_label));
}

void StatusBarWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position(), size(), position(), position() + size(), kStatusBarColor,
                          Layer::kBackground);

    HorizontalLayoutWidget::draw();
}

void StatusBarWidget::set_text(std::string_view str8) {
    line_column_label->set_text(str8);
}

}  // namespace gui
