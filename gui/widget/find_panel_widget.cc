#include "find_panel_widget.h"

#include "gui/renderer/renderer.h"
#include "gui/widget/debug/image_button_widget.h"
#include "gui/widget/debug/text_button_widget.h"

#include <fmt/base.h>

namespace gui {

FindPanelWidget::FindPanelWidget(size_t main_font_id,
                                 size_t ui_font_id,
                                 size_t icon_regex_image_id,
                                 size_t icon_case_sensitive_image_id,
                                 size_t icon_whole_word_image_id,
                                 size_t icon_wrap_image_id,
                                 size_t icon_in_selection_id,
                                 size_t icon_highlight_matches_id,
                                 size_t panel_close_image_id)
    : horizontal_layout(std::make_shared<HorizontalLayoutWidget>(8)),
      text_input_widget(std::make_shared<TextInputWidget>(main_font_id)) {

    auto regex_button = std::make_shared<ImageButtonWidget>(icon_regex_image_id, kIconColor,
                                                            kIconBackgroundFocusedColor, 4);
    auto case_sensitive_button = std::make_shared<ImageButtonWidget>(
        icon_case_sensitive_image_id, kIconColor, kIconBackgroundFocusedColor, 4);
    auto whole_word_button = std::make_shared<ImageButtonWidget>(
        icon_whole_word_image_id, kIconColor, kIconBackgroundFocusedColor, 4);
    auto wrap_button = std::make_shared<ImageButtonWidget>(icon_wrap_image_id, kIconColor,
                                                           kIconBackgroundFocusedColor, 4);
    auto in_selection_button = std::make_shared<ImageButtonWidget>(
        icon_in_selection_id, kIconColor, kIconBackgroundFocusedColor, 4);
    auto highlight_matches_button = std::make_shared<ImageButtonWidget>(
        icon_highlight_matches_id, kIconColor, kIconBackgroundFocusedColor, 4);
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

    auto panel_close_button =
        std::make_shared<ImageButtonWidget>(panel_close_image_id, kCloseIconColor, Rgba{}, 0);
    panel_close_button->setAutoresizing(false);
    horizontal_layout->addChildEnd(panel_close_button);

    auto find_all_button = std::make_shared<TextButtonWidget>(
        ui_font_id, "Find All", kIconBackgroundColor, app::Size{20, 8}, app::Size{200, 52});
    auto find_prev_button = std::make_shared<TextButtonWidget>(
        ui_font_id, "Find Prev", kIconBackgroundColor, app::Size{20, 8}, app::Size{200, 52});
    auto find_button = std::make_shared<TextButtonWidget>(ui_font_id, "Find", kIconBackgroundColor,
                                                          app::Size{20, 8}, app::Size{200, 52});
    find_all_button->setAutoresizing(false);
    find_prev_button->setAutoresizing(false);
    find_button->setAutoresizing(false);
    horizontal_layout->addChildEnd(find_all_button);
    horizontal_layout->addChildEnd(find_prev_button);
    horizontal_layout->addChildEnd(find_button);

    text_input_widget->setAutoresizing(false);
    horizontal_layout->setMainWidget(text_input_widget);

    size.height += text_input_widget->getSize().height;
    size.height += kVerticalPadding * 2;  // Pad top and bottom.
}

void FindPanelWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position, size, kFindPanelColor, Layer::kTwo);

    horizontal_layout->draw();
}

void FindPanelWidget::layout() {
    auto layout_size = size;
    auto pos = position;

    // Horizontal padding.
    layout_size.width -= kHorizontalPadding * 2;
    pos.x += kHorizontalPadding;
    // Top padding.
    pos.y += kVerticalPadding;

    horizontal_layout->setSize(layout_size);
    horizontal_layout->setPosition(pos);
}

}  // namespace gui
