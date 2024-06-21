#include "renderer/renderer.h"
#include "text_view_widget.h"

namespace gui {

TextViewWidget::TextViewWidget(const renderer::Size& size) : Widget{size} {}

void TextViewWidget::draw(const renderer::Size& screen_size, const renderer::Point& offset) {
    int longest_line = 0;
    renderer::Point end_caret_pos;
    // renderer::g_renderer->getTextRenderer().renderText(size, scroll_offset, buffer, offset,
    // end_caret,
    //                                        end_caret, longest_line, end_caret_pos);
    renderer::g_renderer->getTextRenderer().renderText(screen_size, scroll_offset, buffer, offset,
                                                       end_caret, end_caret, longest_line,
                                                       end_caret_pos);
}

void TextViewWidget::scroll(const renderer::Point& delta) {
    // scroll_offset.x += delta.x;
    scroll_offset.y += delta.y;
    if (scroll_offset.y < 0) {
        scroll_offset.y = 0;
    }
}

void TextViewWidget::setContents(const std::string& text) {
    buffer.setContents(text);
}

}
