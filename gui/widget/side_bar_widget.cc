#include "side_bar_widget.h"

#include "gui/renderer/renderer.h"

#include <cmath>

// TODO: Debug use; remove this.
#include <fmt/base.h>

namespace gui {

SideBarWidget::SideBarWidget(int width)
    : ScrollableWidget({.width = width}),
      // TODO: Move this out of here. Or, consider allowing font usage via (name, size) without
      // needing to pass around a font ID everywhere.
      label_font_id{rasterizer().add_system_font(kLabelFontSize, font::FontStyle::kBold)},
      folders_label_font_id{
          rasterizer().add_system_font(kFoldersLabelFontSize, font::FontStyle::kBold)} {
    update_max_scroll();
}

void SideBarWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position(), size(), position(), position() + size(), kSideBarColor,
                          Layer::kBackground);

    const auto& metrics = rasterizer().metrics(label_font_id);

    render_folder_label();
    render_labels();

    size_t visible_lines = std::ceil(static_cast<double>(size().height) / metrics.line_height);
    render_scroll_bars(metrics.line_height, visible_lines);
}

bool SideBarWidget::mouse_position_changed(const std::optional<Point>& mouse_pos) {
    auto old_index = hovered_index;

    if (!mouse_pos) {
        hovered_index = std::nullopt;
        return hovered_index != old_index;
    }

    const auto& metrics = rasterizer().metrics(label_font_id);
    int label_line_height = metrics.line_height;
    for (size_t line = 0; line < strs.size(); ++line) {
        Point coords = position() - scroll_offset;
        coords.y += static_cast<int>(line) * label_line_height;

        if (mouse_pos) {
            if ((coords.x <= mouse_pos->x && mouse_pos->x < coords.x + size().width) &&
                (coords.y <= mouse_pos->y && mouse_pos->y < coords.y + label_line_height)) {
                hovered_index = line;
                return hovered_index != old_index;
            }
        }
    }

    hovered_index = std::nullopt;
    return hovered_index != old_index;
}

void SideBarWidget::update_max_scroll() {
    const auto& metrics = rasterizer().metrics(folders_label_font_id);

    int line_count = strs.size() + 1;
    max_scroll_offset.y = line_count * metrics.line_height;
}

void SideBarWidget::render_folder_label() {
    auto& texture_renderer = Renderer::instance().getTextureRenderer();
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();

    const auto& layout = line_layout_cache.get(folders_label_font_id, "FOLDERS");

    Point text_coords = position() - scroll_offset;
    text_coords.x += kLeftPadding;
    text_coords.y += kTopPadding;
    // TODO: Debug use; change this.
    const auto highlight_callback = [](size_t) { return Rgb{255, 127, 0}; };

    Point min_coords = {
        .x = scroll_offset.x - kLeftPadding,
        .y = position().y,
    };
    Point max_coords = {
        .x = scroll_offset.x + size().width - kLeftPadding,
        .y = position().y + size().height,
    };
    texture_renderer.addLineLayout(layout, text_coords, min_coords, max_coords,
                                   highlight_callback);
}

void SideBarWidget::render_labels() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    auto& texture_renderer = Renderer::instance().getTextureRenderer();
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();

    // TODO: Experimental; formalize this.
    const auto& metrics = rasterizer().metrics(folders_label_font_id);
    int label_line_height = metrics.line_height;
    for (size_t line = 0; line < strs.size(); ++line) {
        const auto& layout = line_layout_cache.get(label_font_id, strs[line]);

        Point coords = position() - scroll_offset;
        // TODO: Formalize the +1 from the "FOLDERS" label being rendered above.
        coords.y += static_cast<int>(line + 1) * label_line_height;

        Point text_coords = coords;
        text_coords.x += kLeftPadding;
        text_coords.y += kTopPadding;
        const auto highlight_callback = [](size_t) { return kTextColor; };

        Point min_coords = {
            .x = scroll_offset.x - kLeftPadding,
            .y = position().y,
        };
        Point max_coords = {
            .x = scroll_offset.x + size().width - kLeftPadding,
            .y = position().y + size().height,
        };
        texture_renderer.addLineLayout(layout, text_coords, min_coords, max_coords,
                                       highlight_callback);

        // Highlight on mouse hover.
        if (line == hovered_index) {
            Size highlight_size = {size().width, label_line_height};
            rect_renderer.addRect(coords, highlight_size, position(), position() + size(),
                                  kHighlightColor, Layer::kBackground);
        }
    }
}

void SideBarWidget::render_scroll_bars(int line_height, size_t visible_lines) {
    auto& rect_renderer = Renderer::instance().getRectRenderer();

    // Add vertical scroll bar.
    int vbar_width = 15;
    double max_scrollbar_y = size().height + strs.size() * line_height;
    double vbar_height_percent = static_cast<double>(size().height) / max_scrollbar_y;
    int vbar_height = static_cast<int>(size().height * vbar_height_percent);
    vbar_height = std::max(30, vbar_height);
    double vbar_percent = static_cast<double>(scroll_offset.y) / max_scroll_offset.y;
    Point vbar_coords = {
        .x = size().width - vbar_width,
        .y = static_cast<int>(std::round((size().height - vbar_height) * vbar_percent)),
    };
    vbar_coords += position();
    Size vbar_size = {vbar_width, vbar_height};
    rect_renderer.addRect(vbar_coords, vbar_size, position(), position() + size(), kScrollBarColor,
                          Layer::kForeground, 5);
}

}  // namespace gui
