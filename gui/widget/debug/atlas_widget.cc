#include "atlas_widget.h"

// TODO: Debug use; remove this.
#include <random>

namespace gui {

AtlasWidget::AtlasWidget() : ScrollableWidget({.width = 400}) {
    const auto& glyph_cache = Renderer::instance().getGlyphCache();
    size_t count = glyph_cache.pageCount();
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
    const auto& glyph_cache = Renderer::instance().getGlyphCache();

    rect_renderer.addRect(position, size, kSideBarColor, Layer::kBackground);

    const app::Size atlas_size = {
        .width = Atlas::kAtlasSize,
        .height = Atlas::kAtlasSize,
    };

    size_t count = glyph_cache.pageCount();

    // TODO: Refactor this ugly hack.
    while (page_colors.size() < count) {
        page_colors.emplace_back(RandomColor());
        max_scroll_offset.y = page_colors.size() * Atlas::kAtlasSize;
    }

    int min_x = position.x;
    int max_x = position.x + size.width;
    int min_y = position.y;
    int max_y = position.y + size.height;

    auto coords = position - scroll_offset;
    for (size_t page = 0; page < count; ++page) {
        texture_renderer.renderAtlasPage(page, coords, min_x, max_x, min_y, max_y);
        rect_renderer.addRect(coords, atlas_size, page_colors[page], Layer::kBackground, 0, 0,
                              min_x, max_x, min_y, max_y);

        coords.y += Atlas::kAtlasSize;
    }
}

void AtlasWidget::updateMaxScroll() {}

}  // namespace gui
