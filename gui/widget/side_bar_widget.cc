#include "gui/renderer/renderer.h"
#include "side_bar_widget.h"
#include <cmath>

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

void SideBarWidget::draw(const Point& mouse_pos) {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position, size, kSideBarColor, RectRenderer::RectLayer::kForeground);

    // const auto& metrics = rasterizer().getMetrics(label_font_id);
    // renderOldLabel(metrics.line_height);

    renderNewLabel(mouse_pos);
    // renderScrollBars();
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

void SideBarWidget::renderNewLabel(const Point& mouse_pos) {
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
        text_renderer.renderLineLayout(layout, text_coords, min_x, max_x, {51, 51, 51},
                                       TextRenderer::TextLayer::kBackground);

        // Highlight on mouse hover.
        if ((coords.x <= mouse_pos.x && mouse_pos.x < coords.x + size.width) &&
            (coords.y <= mouse_pos.y && mouse_pos.y < coords.y + label_line_height)) {
            rect_renderer.addRect(coords, {size.width, label_line_height}, {255, 255, 0, 255},
                                  RectRenderer::RectLayer::kForeground);
        }
    }
}

void SideBarWidget::renderScrollBars() {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();

    // Add vertical scroll bar.
    int vbar_width = 15;
    int vbar_height = size.height * (static_cast<float>(size.height) / max_scroll_offset.y);
    float vbar_percent = static_cast<float>(scroll_offset.y) / max_scroll_offset.y;

    Point coords{
        .x = size.width - vbar_width,
        .y = static_cast<int>(std::round((size.height - vbar_height) * vbar_percent)),
    };
    rect_renderer.addRect(coords + position, {vbar_width, vbar_height}, kScrollBarColor,
                          RectRenderer::RectLayer::kForeground, 5);
}

}
