#include "atlas_widget.h"

namespace gui {

AtlasWidget::AtlasWidget() : ScrollableWidget({.width = Atlas::kAtlasSize}) {
    // max_scroll_offset.y = 2000;
}

void AtlasWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    auto& text_renderer = Renderer::instance().getTextRenderer();

    rect_renderer.addRect(position, size, kSideBarColor, Layer::kOne);

    const app::Size atlas_size = {
        .width = Atlas::kAtlasSize,
        .height = Atlas::kAtlasSize,
    };

    auto coords = position - scroll_offset;
    text_renderer.renderAtlasPages(coords);
    rect_renderer.addRect(coords, atlas_size, {255, 127, 0}, Layer::kOne);
}

void AtlasWidget::updateMaxScroll() {}

}  // namespace gui
