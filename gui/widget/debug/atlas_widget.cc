#include "atlas_widget.h"

// TODO: Debug use; remove this.
#include <random>

namespace gui {

AtlasWidget::AtlasWidget() : ScrollableWidget({.width = 400}) {
    const auto& texture_cache = Renderer::instance().getTextureCache();
    size_t count = texture_cache.pageCount();
    max_scroll_offset.x = Atlas::kAtlasSize;
    max_scroll_offset.y = count * Atlas::kAtlasSize;
}

namespace {
inline Rgba RandomColor() {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, 255);
    uint8_t r = dist(rng);
    uint8_t g = dist(rng);
    uint8_t b = dist(rng);
    return {r, g, b};
}
}  // namespace

void AtlasWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    auto& texture_renderer = Renderer::instance().getTextureRenderer();
    const auto& texture_cache = Renderer::instance().getTextureCache();

    rect_renderer.addRect(position, size, position, position + size, kSideBarColor,
                          Layer::kBackground);

    const app::Size atlas_size = {
        .width = Atlas::kAtlasSize,
        .height = Atlas::kAtlasSize,
    };

    size_t count = texture_cache.pageCount();

    // TODO: Refactor this ugly hack.
    while (page_colors.size() < count) {
        page_colors.emplace_back(RandomColor());
        max_scroll_offset.y = page_colors.size() * Atlas::kAtlasSize;
    }

    auto coords = position - scroll_offset;
    for (size_t page = 0; page < count; ++page) {
        texture_renderer.renderAtlasPage(page, coords, position, position + size);
        rect_renderer.addRect(coords, atlas_size, position, position + size, page_colors[page],
                              Layer::kBackground, 0, 0);

        coords.y += Atlas::kAtlasSize;
    }
}

void AtlasWidget::updateMaxScroll() {}

}  // namespace gui
