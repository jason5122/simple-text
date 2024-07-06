#include "renderer/renderer.h"
#include "side_bar_widget.h"
#include <cmath>

namespace gui {

SideBarWidget::SideBarWidget(const renderer::Size& size) : Widget{size} {}

void SideBarWidget::draw() {
    renderer::TextRenderer& text_renderer = renderer::g_renderer->getTextRenderer();
    renderer::RectRenderer& rect_renderer = renderer::g_renderer->getRectRenderer();
    renderer::ImageRenderer& image_renderer = renderer::g_renderer->getImageRenderer();

    rect_renderer.addRect(position, size, {235, 237, 239, 255});

    // Add vertical scroll bar.
    int vbar_width = 15;
    int max_scrollbar_y = 2688;
    int vbar_height = size.height * (static_cast<float>(size.height) / max_scrollbar_y);
    float vbar_percent = static_cast<float>(scroll_offset.y) / 1176;

    renderer::Point coords{
        .x = size.width - vbar_width,
        .y = static_cast<int>(std::round((size.height - vbar_height) * vbar_percent)),
    };
    rect_renderer.addRoundedRect(coords + position, {vbar_width, vbar_height},
                                 {190, 190, 190, 255}, 5);

    // Add folder icons.
    image_renderer.addImage(renderer::ImageRenderer::kFolderOpen2xIndex, position - scroll_offset,
                            {142, 142, 142, 255});

    // Add side bar text.
    renderer::Size image_size =
        image_renderer.getImageSize(renderer::ImageRenderer::kFolderOpen2xIndex);
    renderer::Point text_coords = position - scroll_offset;
    text_coords.x += image_size.width;
    text_renderer.addUiText(text_coords, kFoldersText);
}

void SideBarWidget::scroll(const renderer::Point& mouse_pos, const renderer::Point& delta) {
    int max_scrollbar_y = 1176;

    // scroll_offset.x += delta.x;
    scroll_offset.y += delta.y;
    scroll_offset.y = std::clamp(scroll_offset.y, 0, max_scrollbar_y);
}

}
