#include "find_panel_widget.h"

#include "gui/renderer/renderer.h"

namespace gui {

FindPanelWidget::FindPanelWidget(const app::Size& size) : Widget{size} {}

void FindPanelWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position, size, kFindPanelColor, RectRenderer::RectLayer::kForeground);

    app::Point text_input_pos = position;
    text_input_pos.y += 6 * 2;
    app::Size text_input_width = {size.width, 60};
    rect_renderer.addRect(text_input_pos, text_input_width, kTextInputColor,
                          RectRenderer::RectLayer::kForeground);
}

}  // namespace gui
