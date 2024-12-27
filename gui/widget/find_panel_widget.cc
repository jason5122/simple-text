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
    : HorizontalLayoutWidget(8, kHorizontalPadding, kHorizontalPadding, kVerticalPadding),
      text_input_widget(std::make_shared<TextInputWidget>(main_font_id, 8, 10)) {

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
    addChildStart(regex_button);
    addChildStart(case_sensitive_button);
    addChildStart(whole_word_button);
    addChildStart(wrap_button);
    addChildStart(in_selection_button);
    addChildStart(highlight_matches_button);

    auto panel_close_button =
        std::make_shared<ImageButtonWidget>(panel_close_image_id, kCloseIconColor, Rgba{}, 0);
    panel_close_button->setAutoresizing(false);
    addChildEnd(panel_close_button);

    auto find_all_button = std::make_shared<TextButtonWidget>(
        ui_font_id, "Find All", kIconBackgroundColor, app::Size{20, 8}, app::Size{200, 52});
    auto find_prev_button = std::make_shared<TextButtonWidget>(
        ui_font_id, "Find Prev", kIconBackgroundColor, app::Size{20, 8}, app::Size{200, 52});
    auto find_button = std::make_shared<TextButtonWidget>(ui_font_id, "Find", kIconBackgroundColor,
                                                          app::Size{20, 8}, app::Size{200, 52});
    find_all_button->setAutoresizing(false);
    find_prev_button->setAutoresizing(false);
    find_button->setAutoresizing(false);
    addChildEnd(find_all_button);
    addChildEnd(find_prev_button);
    addChildEnd(find_button);

    text_input_widget->setAutoresizing(false);
    setMainWidget(text_input_widget);

    // Calculate max height. We assume all buttons have the same height.
    int text_input_height = text_input_widget->getSize().height;
    text_input_height += kVerticalPadding * 2;  // Pad top and bottom.

    int button_height = find_button->getSize().height;
    button_height += kVerticalPadding * 2;  // Pad top and bottom.

    size.height = std::max(text_input_height, button_height);
}

void FindPanelWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position, size, kFindPanelColor, Layer::kTwo);

    HorizontalLayoutWidget::draw();
}

}  // namespace gui
