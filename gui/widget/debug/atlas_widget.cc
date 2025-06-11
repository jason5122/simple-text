#include "gui/renderer/renderer.h"
#include "gui/widget/debug/atlas_widget.h"
#include <random>

namespace gui {

AtlasWidget::AtlasWidget() : ScrollableWidget({.width = 400}) {
    const auto& texture_cache = Renderer::instance().texture_cache();
    size_t count = texture_cache.pages().size();
    max_scroll_offset.x = Atlas::kAtlasSize;
    max_scroll_offset.y = count * Atlas::kAtlasSize;
}

namespace {
inline Rgb random_color() {
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
    auto& rect_renderer = Renderer::instance().rect_renderer();
    auto& texture_renderer = Renderer::instance().texture_renderer();
    const auto& texture_cache = Renderer::instance().texture_cache();

    const auto min_coords = position();
    const auto max_coords = min_coords + size();

    rect_renderer.add_rect(position(), size(), min_coords, max_coords, kSideBarColor,
                           Layer::kBackground);

    const Size atlas_size = {
        .width = Atlas::kAtlasSize,
        .height = Atlas::kAtlasSize,
    };

    size_t count = texture_cache.pages().size();

    // TODO: Refactor this ugly hack.
    while (page_colors.size() < count) {
        page_colors.emplace_back(random_color());
        max_scroll_offset.y = page_colors.size() * Atlas::kAtlasSize;
    }

    auto coords = position() - scroll_offset;
    for (size_t page = 0; page < count; ++page) {
        texture_renderer.render_atlas_page(page, coords, min_coords, max_coords);
        rect_renderer.add_rect(coords, atlas_size, min_coords, max_coords, page_colors[page],
                               Layer::kBackground, 0, 0);

        coords.y += Atlas::kAtlasSize;
    }
}

void AtlasWidget::update_max_scroll() {}

}  // namespace gui
