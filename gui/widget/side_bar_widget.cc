#include "gui/renderer/renderer.h"
#include "side_bar_widget.h"
#include <cmath>

// TODO: Debug use; remove this.
#include "util/std_print.h"

namespace gui {

SideBarWidget::SideBarWidget(const Size& size)
    : ScrollableWidget{size},
      label_font_id{
          rasterizer().addFont("SF Pro Text", 22 * 2, font::FontRasterizer::FontStyle::kBold)},
      line_layout_cache{label_font_id} {
    updateMaxScroll();

    // const auto& metrics = rasterizer().getMetrics(label_font_id);
    // int label_line_height = metrics.line_height;

    // folder_label.reset(new LabelWidget{{size.width, label_line_height}});
    // folder_label->setText("FOLDERS", {128, 128, 128});
    // folder_label->addLeftIcon(ImageRenderer::kFolderOpen2xIndex);
}

void SideBarWidget::draw(const std::optional<Point>& mouse_pos) {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position, size, kSideBarColor, RectRenderer::RectLayer::kForeground);

    const auto& metrics = rasterizer().getMetrics(label_font_id);
    // renderOldLabel(metrics.line_height);

    renderNewLabel(mouse_pos);

    size_t visible_lines = std::ceil(static_cast<double>(size.height) / metrics.line_height);
    renderScrollBars(metrics.line_height, visible_lines);
}

void SideBarWidget::mousePositionChanged(const std::optional<Point>& mouse_pos) {
    if (!mouse_pos) {
        hovered_index = std::nullopt;
        return;
    }

    const auto& metrics = rasterizer().getMetrics(label_font_id);
    int label_line_height = metrics.line_height;
    for (size_t line = 0; line < strs.size(); ++line) {
        Point coords = position - scroll_offset;
        coords.y += static_cast<int>(line) * label_line_height;

        if (mouse_pos) {
            if ((coords.x <= mouse_pos->x && mouse_pos->x < coords.x + size.width) &&
                (coords.y <= mouse_pos->y && mouse_pos->y < coords.y + label_line_height)) {
                hovered_index = line;
                return;
            }
        }
    }

    hovered_index = std::nullopt;
}

void SideBarWidget::layout() {
    // Point label_pos = position - scroll_offset;
    // label_pos.y += 400;
    // folder_label->setPosition(label_pos);
}

void SideBarWidget::updateMaxScroll() {
    const auto& metrics = rasterizer().getMetrics(label_font_id);

    int line_count = strs.size() + 1;
    max_scroll_offset.y = line_count * metrics.line_height;
}

void SideBarWidget::renderOldLabel(int label_line_height) {
    // folder_label->draw({});  // TODO: Make mouse position optional.
    // RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();
    // rect_renderer.addRect(folder_label->getPosition(), {size.width, label_line_height},
    //                       {255, 255, 0, 255}, RectRenderer::RectType::kForeground);
}

void SideBarWidget::renderNewLabel(const std::optional<Point>& mouse_pos) {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();
    TextRenderer& text_renderer = Renderer::instance().getTextRenderer();

    // TODO: Experimental; formalize this.
    const auto& metrics = rasterizer().getMetrics(label_font_id);
    int label_line_height = metrics.line_height;
    for (size_t line = 0; line < strs.size(); ++line) {
        const auto& layout = line_layout_cache.getLineLayout(strs[line]);

        Point coords = position - scroll_offset;
        coords.y += static_cast<int>(line) * label_line_height;

        int min_x = scroll_offset.x;
        int max_x = scroll_offset.x + size.width;
        Point text_coords = coords;
        text_coords.x += kLeftPadding;
        const auto highlight_callback = [](size_t) { return kTextColor; };
        text_renderer.renderLineLayout(layout, text_coords, TextRenderer::TextLayer::kBackground,
                                       highlight_callback, min_x, max_x);

        // Highlight on mouse hover.
        if (hovered_index && line == hovered_index) {
            rect_renderer.addRect(coords, {size.width, label_line_height}, {255, 255, 0, 255},
                                  RectRenderer::RectLayer::kForeground);
        }
    }
}

void SideBarWidget::renderScrollBars(int line_height, size_t visible_lines) {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();

    // Add vertical scroll bar.
    int vbar_width = 15;
    double max_scrollbar_y = size.height + strs.size() * line_height;
    double vbar_height_percent = static_cast<double>(size.height) / max_scrollbar_y;
    int vbar_height = static_cast<int>(size.height * vbar_height_percent);
    vbar_height = std::max(30, vbar_height);
    double vbar_percent = static_cast<double>(scroll_offset.y) / max_scroll_offset.y;
    Point vbar_coords{
        .x = size.width - vbar_width,
        .y = static_cast<int>(std::round((size.height - vbar_height) * vbar_percent)),
    };
    rect_renderer.addRect(vbar_coords + position, {vbar_width, vbar_height}, kScrollBarColor,
                          RectRenderer::RectLayer::kForeground, 5);
}

}
