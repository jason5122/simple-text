#include "side_bar_widget.h"

#include "gui/renderer/renderer.h"

#include <cmath>

// TODO: Debug use; remove this.
#include <fmt/base.h>

namespace gui {

SideBarWidget::SideBarWidget(int width)
    : ScrollableWidget({.width = width}),
      label_font_id{rasterizer().addSystemFont(kLabelFontSize, font::FontStyle::kBold)} {
    updateMaxScroll();
}

void SideBarWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position, size, kSideBarColor, Layer::kBackground);

    const auto& metrics = rasterizer().metrics(label_font_id);

    renderLabel();

    size_t visible_lines = std::ceil(static_cast<double>(size.height) / metrics.line_height);
    renderScrollBars(metrics.line_height, visible_lines);
}

bool SideBarWidget::mousePositionChanged(const std::optional<app::Point>& mouse_pos) {
    auto old_index = hovered_index;

    if (!mouse_pos) {
        hovered_index = std::nullopt;
        return hovered_index != old_index;
    }

    const auto& metrics = rasterizer().metrics(label_font_id);
    int label_line_height = metrics.line_height;
    for (size_t line = 0; line < strs.size(); ++line) {
        app::Point coords = position - scroll_offset;
        coords.y += static_cast<int>(line) * label_line_height;

        if (mouse_pos) {
            if ((coords.x <= mouse_pos->x && mouse_pos->x < coords.x + size.width) &&
                (coords.y <= mouse_pos->y && mouse_pos->y < coords.y + label_line_height)) {
                hovered_index = line;
                return hovered_index != old_index;
            }
        }
    }

    hovered_index = std::nullopt;
    return hovered_index != old_index;
}

void SideBarWidget::updateMaxScroll() {
    const auto& metrics = rasterizer().metrics(label_font_id);

    int line_count = strs.size() + 1;
    max_scroll_offset.y = line_count * metrics.line_height;
}

void SideBarWidget::renderLabel() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    auto& texture_renderer = Renderer::instance().getTextureRenderer();
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();

    // TODO: Experimental; formalize this.
    const auto& metrics = rasterizer().metrics(label_font_id);
    int label_line_height = metrics.line_height;
    for (size_t line = 0; line < strs.size(); ++line) {
        const auto& layout = line_layout_cache.get(label_font_id, strs[line]);

        app::Point coords = position - scroll_offset;
        coords.y += static_cast<int>(line) * label_line_height;

        app::Point text_coords = coords;
        text_coords.x += kLeftPadding;
        const auto highlight_callback = [](size_t) { return kTextColor; };

        int min_x = scroll_offset.x - kLeftPadding;
        int max_x = scroll_offset.x + size.width - kLeftPadding;
        texture_renderer.insertLineLayout(layout, text_coords, highlight_callback, min_x, max_x);

        // Highlight on mouse hover.
        if (line == hovered_index) {
            rect_renderer.addRect(coords, {size.width, label_line_height}, {255, 255, 0, 255},
                                  Layer::kBackground);
        }
    }
}

void SideBarWidget::renderScrollBars(int line_height, size_t visible_lines) {
    auto& rect_renderer = Renderer::instance().getRectRenderer();

    // Add vertical scroll bar.
    int vbar_width = 15;
    double max_scrollbar_y = size.height + strs.size() * line_height;
    double vbar_height_percent = static_cast<double>(size.height) / max_scrollbar_y;
    int vbar_height = static_cast<int>(size.height * vbar_height_percent);
    vbar_height = std::max(30, vbar_height);
    double vbar_percent = static_cast<double>(scroll_offset.y) / max_scroll_offset.y;
    app::Point vbar_coords{
        .x = size.width - vbar_width,
        .y = static_cast<int>(std::round((size.height - vbar_height) * vbar_percent)),
    };
    rect_renderer.addRect(vbar_coords + position, {vbar_width, vbar_height}, kScrollBarColor,
                          Layer::kForeground, 5);
}

}  // namespace gui
