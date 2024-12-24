#include "find_panel_widget.h"

#include "gui/renderer/renderer.h"

#include <fmt/base.h>

namespace gui {

FindPanelWidget::FindPanelWidget(const app::Size& size, size_t font_id)
    : ScrollableWidget(size), font_id(font_id) {
    tree.insert(0, "needle");
}

void FindPanelWidget::draw() {
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);
    int line_height = metrics.line_height;

    auto& rect_renderer = Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position, size, kFindPanelColor, RectRenderer::RectLayer::kBackground);

    app::Point text_input_pos = position;
    text_input_pos.y += 6 * 2;
    app::Size text_input_size = {size.width, line_height};
    text_input_size.height += kTextInputPadding * 2;  // Pad top and bottom.
    text_input_size.height += 2;                      // Add padding for selection border.

    rect_renderer.addRect(text_input_pos, text_input_size, kTextInputColor,
                          RectRenderer::RectLayer::kBackground);

    // Render text.
    auto& text_renderer = Renderer::instance().getTextRenderer();

    int min_x = scroll_offset.x;
    int max_x = scroll_offset.x + size.width;

    for (size_t line = 0; line < tree.line_count(); ++line) {
        const auto& layout = layoutAt(line);
        app::Point coords = text_input_pos;
        coords.y += static_cast<int>(line) * line_height;
        coords.y += 6;  // Pad top.
        coords.y += 2;  // Selection border padding?

        text_renderer.renderLineLayout(
            layout, coords, TextRenderer::TextLayer::kBackground,
            [](size_t) { return kTextColor; }, min_x, max_x);
    }

    // Render selections.
    auto& selection_renderer = Renderer::instance().getSelectionRenderer();
    std::vector<SelectionRenderer::Selection> selections;
    for (size_t line = 0; line < tree.line_count(); ++line) {
        const auto& layout = layoutAt(line);
        selections.emplace_back(SelectionRenderer::Selection{
            .line = static_cast<int>(line),
            .start = 0,
            .end = layout.width,
        });
    }

    app::Point temp = text_input_pos;
    temp.y += static_cast<int>(0) * line_height;
    temp.y += 6;  // Pad top.
    temp.y += 2;  // Selection border padding?
    selection_renderer.renderSelections(selections, temp, line_height);
}

void FindPanelWidget::updateMaxScroll() {}

inline const font::LineLayout& FindPanelWidget::layoutAt(size_t line) {
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    std::string line_str = tree.get_line_content_for_layout_use(line);
    return line_layout_cache.get(font_id, line_str);
}

}  // namespace gui
