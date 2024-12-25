#include "find_panel_widget.h"

#include "gui/renderer/renderer.h"

#include <fmt/base.h>

namespace gui {

FindPanelWidget::FindPanelWidget(const app::Size& size,
                                 size_t font_id,
                                 size_t icon_regex_image_id,
                                 size_t icon_case_sensitive_image_id)
    : ScrollableWidget(size),
      font_id(font_id),
      icon_regex_image_id(icon_regex_image_id),
      icon_case_sensitive_image_id(icon_case_sensitive_image_id) {
    tree.insert(0, "needle");

    auto& image_renderer = Renderer::instance().getImageRenderer();
    auto& regex_image = image_renderer.get(icon_regex_image_id);
    image_offset_x += regex_image.width;
    image_offset_x += 4 * 2;
    fmt::println("regex_image.width = {}", regex_image.width);

    auto& case_sensitive_image = image_renderer.get(icon_case_sensitive_image_id);
    image_offset_x += case_sensitive_image.width;
    image_offset_x += 4 * 2;
    fmt::println("case_sensitive_image.width = {}", case_sensitive_image.width);
}

void FindPanelWidget::draw() {
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);
    int line_height = metrics.line_height;

    auto& rect_renderer = Renderer::instance().getRectRenderer();
    auto& image_renderer = Renderer::instance().getImageRenderer();
    auto& text_renderer = Renderer::instance().getTextRenderer();
    auto& selection_renderer = Renderer::instance().getSelectionRenderer();

    rect_renderer.addRect(position, size, kFindPanelColor, RectRenderer::RectLayer::kBackground);

    app::Size text_input_size = {size.width, line_height};
    text_input_size.height += kTextInputPadding * 2;  // Pad top and bottom.
    text_input_size.height += 2;                      // Add padding for selection border.

    rect_renderer.addRect(textInputOffset(), text_input_size, kTextInputColor,
                          RectRenderer::RectLayer::kBackground);

    renderText(line_height);
    renderSelections(line_height);
    renderImages();
}

void FindPanelWidget::updateMaxScroll() {}

inline const font::LineLayout& FindPanelWidget::layoutAt(size_t line) {
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    std::string line_str = tree.get_line_content_for_layout_use(line);
    return line_layout_cache.get(font_id, line_str);
}

constexpr app::Point FindPanelWidget::textInputOffset() {
    app::Point text_input_pos = position;
    text_input_pos += app::Point{6 * 2, 6 * 2};
    text_input_pos.x += image_offset_x;
    return text_input_pos;
}

void FindPanelWidget::renderText(int line_height) {
    auto& text_renderer = Renderer::instance().getTextRenderer();
    int min_x = scroll_offset.x;
    int max_x = scroll_offset.x + size.width;

    for (size_t line = 0; line < tree.line_count(); ++line) {
        const auto& layout = layoutAt(line);
        app::Point coords = textInputOffset();
        coords.y += static_cast<int>(line) * line_height;
        coords.y += 6;  // Pad top.
        coords.y += 2;  // Selection border padding?

        text_renderer.renderLineLayout(
            layout, coords, TextRenderer::TextLayer::kBackground,
            [](size_t) { return kTextColor; }, min_x, max_x);
    }
}

void FindPanelWidget::renderSelections(int line_height) {
    auto& selection_renderer = Renderer::instance().getSelectionRenderer();
    std::vector<SelectionRenderer::Selection> selections;
    for (size_t line = 0; line < tree.line_count(); ++line) {
        const auto& layout = layoutAt(line);
        int start = 0;
        int end = layout.width;

        selections.emplace_back(SelectionRenderer::Selection{
            .line = static_cast<int>(line),
            .start = start,
            .end = end,
        });
    }

    app::Point temp = textInputOffset();
    temp.y += static_cast<int>(0) * line_height;
    temp.y += 6;  // Pad top.
    temp.y += 2;  // Selection border padding?
    selection_renderer.renderSelections(selections, temp, line_height);
}

void FindPanelWidget::renderImages() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    auto& image_renderer = Renderer::instance().getImageRenderer();

    {
        auto& image = image_renderer.get(icon_regex_image_id);
        app::Size image_size = {
            .width = static_cast<int>(image.width),
            .height = static_cast<int>(image.height),
        };
        image_size += app::Size{4 * 2, 4 * 2};

        app::Point rename_this = position;
        rename_this += app::Point{6 * 2, 6 * 2};
        image_renderer.insertInBatch(icon_regex_image_id, rename_this + app::Point{4, 4},
                                     kIconColor);
        rect_renderer.addRect(rename_this, image_size, kIconBackgroundColor,
                              RectRenderer::RectLayer::kBackground, 4);
    }

    {
        auto& image = image_renderer.get(icon_case_sensitive_image_id);
        app::Size image_size = {
            .width = static_cast<int>(image.width),
            .height = static_cast<int>(image.height),
        };
        image_size += app::Size{4 * 2, 4 * 2};

        app::Point rename_this = position;
        rename_this.x += image.width;
        rename_this.x += 4 * 2;
        rename_this += app::Point{6 * 2, 6 * 2};
        image_renderer.insertInBatch(icon_case_sensitive_image_id, rename_this + app::Point{4, 4},
                                     kIconColor);
        rect_renderer.addRect(rename_this, image_size, kIconBackgroundColor,
                              RectRenderer::RectLayer::kBackground, 4);
    }
}

}  // namespace gui
