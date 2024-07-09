#include "label_widget.h"
#include "renderer/renderer.h"

namespace gui {

void LabelWidget::setText(const std::string& str8) {
    label_text.setText(str8);
}

void LabelWidget::draw() {
    renderer::TextRenderer& text_renderer = renderer::Renderer::instance().getTextRenderer();
    renderer::RectRenderer& rect_renderer = renderer::Renderer::instance().getRectRenderer();

    constexpr renderer::Rgba temp_color{223, 227, 230, 255};

    rect_renderer.addRect(position, size, temp_color);
    text_renderer.addUiText(position, label_text);
}

}
