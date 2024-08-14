#include "gui/renderer/renderer.h"
#include "side_bar_widget.h"
#include <cmath>

namespace gui {

SideBarWidget::SideBarWidget(const Size& size)
    : ScrollableWidget{size}, folder_label{new LabelWidget{{size.width, 50}}} {
    updateMaxScroll();

    folder_label->setText("FOLDERS", {51, 51, 51});
    folder_label->addLeftIcon(ImageRenderer::kFolderOpen2xIndex);
}

void SideBarWidget::draw() {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();

    rect_renderer.addRect(position, size, kSideBarColor);

    folder_label->draw();

    // Add vertical scroll bar.
    int vbar_width = 15;
    int vbar_height = size.height * (static_cast<float>(size.height) / max_scroll_offset.y);
    float vbar_percent = static_cast<float>(scroll_offset.y) / max_scroll_offset.y;

    Point coords{
        .x = size.width - vbar_width,
        .y = static_cast<int>(std::round((size.height - vbar_height) * vbar_percent)),
    };
    rect_renderer.addRect(coords + position, {vbar_width, vbar_height}, kScrollBarColor, 5);
}

void SideBarWidget::layout() {
    folder_label->setPosition(position - scroll_offset);
}

void SideBarWidget::updateMaxScroll() {
    // TODO: Debug use; remove this.
    max_scroll_offset.y = 2400;
}

}
