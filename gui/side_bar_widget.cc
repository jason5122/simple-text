#include "renderer/renderer.h"
#include "side_bar_widget.h"
#include <cmath>

namespace gui {

SideBarWidget::SideBarWidget(const renderer::Size& size)
    : ScrollableWidget{size}, folder_label{new LabelWidget{{size.width, 50}}} {
    updateMaxScroll();

    folder_label->setText("FOLDERS");
    folder_label->addLeftIcon(renderer::ImageRenderer::kFolderOpen2xIndex);
}

void SideBarWidget::draw() {
    renderer::TextRenderer& text_renderer = renderer::Renderer::instance().getTextRenderer();
    renderer::RectRenderer& rect_renderer = renderer::Renderer::instance().getRectRenderer();
    renderer::ImageRenderer& image_renderer = renderer::Renderer::instance().getImageRenderer();

    constexpr renderer::Rgba side_bar_color{235, 237, 239, 255};
    constexpr renderer::Rgba scroll_bar_color{190, 190, 190, 255};
    constexpr renderer::Rgba folder_icon_color{142, 142, 142, 255};

    rect_renderer.addRect(position, size, side_bar_color);

    folder_label->draw();

    // Add vertical scroll bar.
    int vbar_width = 15;
    int vbar_height = size.height * (static_cast<float>(size.height) / max_scroll_offset.y);
    float vbar_percent = static_cast<float>(scroll_offset.y) / max_scroll_offset.y;

    renderer::Point coords{
        .x = size.width - vbar_width,
        .y = static_cast<int>(std::round((size.height - vbar_height) * vbar_percent)),
    };
    rect_renderer.addRoundedRect(coords + position, {vbar_width, vbar_height}, scroll_bar_color,
                                 5);
}

void SideBarWidget::layout() {
    folder_label->setPosition(position - scroll_offset);
}

void SideBarWidget::updateMaxScroll() {
    // TODO: Debug use; remove this.
    max_scroll_offset.y = 2688;
}

}
