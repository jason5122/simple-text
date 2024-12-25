#include "find_panel_widget.h"

#include "gui/renderer/renderer.h"
#include "gui/widget/debug/image_button_widget.h"
#include "gui/widget/debug/text_button_widget.h"

#include <fmt/base.h>

namespace gui {

FindPanelWidget::FindPanelWidget(const app::Size& size,
                                 size_t main_font_id,
                                 size_t ui_font_id,
                                 size_t icon_regex_image_id,
                                 size_t icon_case_sensitive_image_id,
                                 size_t icon_whole_word_image_id,
                                 size_t icon_wrap_image_id,
                                 size_t icon_in_selection_id,
                                 size_t icon_highlight_matches_id)
    : ScrollableWidget(size),
      main_font_id(main_font_id),
      horizontal_layout(std::make_shared<HorizontalLayoutWidget>()) {

    auto regex_button =
        std::make_shared<ImageButtonWidget>(icon_regex_image_id, Rgba{255, 0, 0, 255});
    auto case_sensitive_button =
        std::make_shared<ImageButtonWidget>(icon_case_sensitive_image_id, Rgba{255, 255, 0, 255});
    auto whole_word_button =
        std::make_shared<ImageButtonWidget>(icon_whole_word_image_id, Rgba{0, 255, 0, 255});
    auto wrap_button =
        std::make_shared<ImageButtonWidget>(icon_wrap_image_id, Rgba{0, 255, 255, 255});
    auto in_selection_button =
        std::make_shared<ImageButtonWidget>(icon_in_selection_id, Rgba{255, 0, 255, 255});
    auto highlight_matches_button =
        std::make_shared<ImageButtonWidget>(icon_highlight_matches_id, Rgba{255, 127, 0, 255});
    regex_button->setAutoresizing(false);
    case_sensitive_button->setAutoresizing(false);
    whole_word_button->setAutoresizing(false);
    wrap_button->setAutoresizing(false);
    in_selection_button->setAutoresizing(false);
    highlight_matches_button->setAutoresizing(false);
    horizontal_layout->addChildStart(regex_button);
    horizontal_layout->addChildStart(case_sensitive_button);
    horizontal_layout->addChildStart(whole_word_button);
    horizontal_layout->addChildStart(wrap_button);
    horizontal_layout->addChildStart(in_selection_button);
    horizontal_layout->addChildStart(highlight_matches_button);

    auto temp = std::make_shared<TextButtonWidget>(ui_font_id, "Find All", Rgba{255, 127, 0, 255});
    temp->setAutoresizing(false);
    // temp->setSize({200, 52});
    temp->setWidth(200);
    horizontal_layout->addChildEnd(temp);

    tree.insert(0, "needle");

    image_ids.emplace_back(icon_regex_image_id);
    image_ids.emplace_back(icon_case_sensitive_image_id);
    image_ids.emplace_back(icon_whole_word_image_id);
    image_ids.emplace_back(icon_wrap_image_id);
    image_ids.emplace_back(icon_in_selection_id);
    image_ids.emplace_back(icon_highlight_matches_id);

    auto& image_renderer = Renderer::instance().getImageRenderer();
    for (size_t image_id : image_ids) {
        auto& image = image_renderer.get(icon_regex_image_id);
        image_offset_x += image.size.width;
        image_offset_x += 4 * 2;
    }
}

void FindPanelWidget::draw() {
    horizontal_layout->draw();

    // const auto& font_rasterizer = font::FontRasterizer::instance();
    // const auto& metrics = font_rasterizer.metrics(font_id);
    // int line_height = metrics.line_height;

    // auto& rect_renderer = Renderer::instance().getRectRenderer();
    // auto& image_renderer = Renderer::instance().getImageRenderer();
    // auto& text_renderer = Renderer::instance().getTextRenderer();
    // auto& selection_renderer = Renderer::instance().getSelectionRenderer();

    // rect_renderer.addRect(position, size, kFindPanelColor, Layer::kTwo);

    // app::Size text_input_size = {size.width, line_height};
    // text_input_size.height += kTextInputPadding * 2;  // Pad top and bottom.
    // text_input_size.height += 2;                      // Add padding for selection border.

    // rect_renderer.addRect(textInputOffset(), text_input_size, kTextInputColor, Layer::kTwo);

    // renderText(line_height);
    // renderSelections(line_height);
    // renderImages();
}

void FindPanelWidget::layout() {
    horizontal_layout->setSize(size);
    horizontal_layout->setPosition(position);
}

void FindPanelWidget::updateMaxScroll() {}

inline const font::LineLayout& FindPanelWidget::layoutAt(size_t line) {
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    std::string line_str = tree.get_line_content_for_layout_use(line);
    return line_layout_cache.get(main_font_id, line_str);
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
            layout, coords, Layer::kTwo, [](size_t) { return kTextColor; }, min_x, max_x);
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
    selection_renderer.renderSelections(selections, temp, line_height, Layer::kTwo);
}

void FindPanelWidget::renderImages() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    auto& image_renderer = Renderer::instance().getImageRenderer();

    int x = 0;
    for (size_t i = 0; i < image_ids.size(); i++) {
        auto& image = image_renderer.get(image_ids[i]);

        app::Point rename_this = position;
        rename_this += app::Point{6 * 2, 6 * 2};
        rename_this.x += x;

        app::Size image_size = image.size;
        image_size += app::Size{4 * 2, 4 * 2};

        image_renderer.insertInBatch(image_ids[i], rename_this + app::Point{4, 4}, kIconColor,
                                     Layer::kTwo);
        rect_renderer.addRect(rename_this, image_size, kIconBackgroundColor, Layer::kTwo, 4);

        x += image.size.width;
        x += 4 * 2;
    }
}

}  // namespace gui
