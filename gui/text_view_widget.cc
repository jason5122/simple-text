#include "text_view_widget.h"

namespace gui {

TextViewWidget::TextViewWidget(std::shared_ptr<renderer::Renderer> renderer) : Widget{renderer} {}

void TextViewWidget::draw(int width, int height) {
    int longest_line = 0;
    renderer::Point end_caret_pos;
    renderer->getTextRenderer().renderText({width, height}, scroll_offset, buffer,
                                           {200 * 2, 30 * 2}, end_caret, end_caret, longest_line,
                                           end_caret_pos);
}

void TextViewWidget::scroll(int dx, int dy) {
    // scroll_offset.x += dx;
    scroll_offset.y += dy;
    if (scroll_offset.y < 0) {
        scroll_offset.y = 0;
    }
}

void TextViewWidget::setContents(const std::string& text) {
    buffer.setContents(text);
}

}
