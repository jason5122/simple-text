#include "renderer/renderer.h"
#include "text_view_widget.h"

namespace gui {

TextViewWidget::TextViewWidget(const renderer::Size& size) : Widget{size} {}

void TextViewWidget::draw(const renderer::Size& screen_size, const renderer::Point& offset) {
    renderer::TextRenderer& text_renderer = renderer::g_renderer->getTextRenderer();
    renderer::RectRenderer& rect_renderer = renderer::g_renderer->getRectRenderer();

    int longest_line = 0;
    renderer::Point end_caret_pos;
    text_renderer.renderText(screen_size, scroll_offset, buffer, offset, end_caret, end_caret,
                             longest_line, end_caret_pos);

    // Add vertical scroll bar.
    int line_count = buffer.lineCount();
    int line_height = text_renderer.lineHeight();
    int vertical_scroll_bar_width = 15;
    float total_y =
        (line_count + (static_cast<float>(screen_size.height) / line_height)) * line_height;
    int vertical_scroll_bar_height = screen_size.height * (screen_size.height / total_y);
    float vertical_scroll_bar_position_percentage =
        static_cast<float>(scroll_offset.y) / (line_count * line_height);

    renderer::Point coords{
        .x = static_cast<int>(screen_size.width - vertical_scroll_bar_width),
        .y = static_cast<int>(std::round((screen_size.height - vertical_scroll_bar_height) *
                                         vertical_scroll_bar_position_percentage))

    };
    coords += offset;

    rect_renderer.addRoundedRect(coords, {vertical_scroll_bar_width, vertical_scroll_bar_height},
                                 {190, 190, 190, 255}, 5);
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
