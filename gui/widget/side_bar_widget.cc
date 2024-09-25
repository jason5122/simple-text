#include "gui/renderer/renderer.h"
#include "side_bar_widget.h"
#include <cmath>

namespace gui {

SideBarWidget::SideBarWidget(const Size& size)
    : ScrollableWidget{size},
      line_layout_cache{Renderer::instance().getGlyphCache().uiFontId()},
      folder_label{new LabelWidget{{size.width, 50}}} {
    updateMaxScroll();

    folder_label->setText("FOLDERS", {51, 51, 51});
    folder_label->addLeftIcon(ImageRenderer::kFolderOpen2xIndex);
}

void SideBarWidget::draw(const Point& mouse_pos) {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();

    rect_renderer.addRect(position, size, kSideBarColor, RectRenderer::RectType::kForeground);

    // folder_label->draw();

    // Add vertical scroll bar.
    // int vbar_width = 15;
    // int vbar_height = size.height * (static_cast<float>(size.height) / max_scroll_offset.y);
    // float vbar_percent = static_cast<float>(scroll_offset.y) / max_scroll_offset.y;

    // Point coords{
    //     .x = size.width - vbar_width,
    //     .y = static_cast<int>(std::round((size.height - vbar_height) * vbar_percent)),
    // };
    // rect_renderer.addRect(coords + position, {vbar_width, vbar_height}, kScrollBarColor, 5);

    // TODO: Experimental; formalize this.
    const auto& glyph_cache = Renderer::instance().getGlyphCache();
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.getMetrics(glyph_cache.uiFontId());
    TextRenderer& text_renderer = Renderer::instance().getTextRenderer();

    int main_line_height = metrics.line_height;
    for (size_t line = 0; line < strs.size(); ++line) {
        const auto& layout = line_layout_cache.getLineLayout(strs[line]);

        Point coords = position - scroll_offset;
        coords.y += static_cast<int>(line) * main_line_height;

        if (coords.y <= mouse_pos.y && mouse_pos.y < coords.y + main_line_height) {
            rect_renderer.addRect(coords, {size.width, main_line_height}, {255, 255, 0, 255},
                                  RectRenderer::RectType::kForeground);
        }

        int min_x = scroll_offset.x;
        int max_x = scroll_offset.x + size.width;
        text_renderer.renderLineLayout(layout, coords, min_x, max_x, {51, 51, 51},
                                       TextRenderer::FontType::kUI);
    }
}

void SideBarWidget::layout() {
    folder_label->setPosition(position - scroll_offset);
}

void SideBarWidget::updateMaxScroll() {
    const auto& glyph_cache = Renderer::instance().getGlyphCache();
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.getMetrics(glyph_cache.uiFontId());

    int line_count = strs.size() + 1;
    max_scroll_offset.y = line_count * metrics.line_height;
}

}
