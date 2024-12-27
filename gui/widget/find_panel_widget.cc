#include "find_panel_widget.h"

#include "gui/renderer/renderer.h"
#include "gui/widget/image_button_widget.h"
#include "gui/widget/text_button_widget.h"

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
    : HorizontalLayoutWidget(
          8, kHorizontalPadding, kHorizontalPadding, kVerticalPadding, kVerticalPadding),
      text_input_widget(new TextInputWidget(main_font_id, 8, 10)) {

    auto regex_button = std::make_unique<ImageButtonWidget>(icon_regex_image_id, kIconColor,
                                                            kIconBackgroundFocusedColor, 4);
    auto case_sensitive_button = std::make_unique<ImageButtonWidget>(
        icon_case_sensitive_image_id, kIconColor, kIconBackgroundFocusedColor, 4);
    auto whole_word_button = std::make_unique<ImageButtonWidget>(
        icon_whole_word_image_id, kIconColor, kIconBackgroundFocusedColor, 4);
    auto wrap_button = std::make_unique<ImageButtonWidget>(icon_wrap_image_id, kIconColor,
                                                           kIconBackgroundFocusedColor, 4);
    auto in_selection_button = std::make_unique<ImageButtonWidget>(
        icon_in_selection_id, kIconColor, kIconBackgroundFocusedColor, 4);
    auto highlight_matches_button = std::make_unique<ImageButtonWidget>(
        icon_highlight_matches_id, kIconColor, kIconBackgroundFocusedColor, 4);
    regex_button->setAutoresizing(false);
    case_sensitive_button->setAutoresizing(false);
    whole_word_button->setAutoresizing(false);
    wrap_button->setAutoresizing(false);
    in_selection_button->setAutoresizing(false);
    highlight_matches_button->setAutoresizing(false);
    addChildStart(std::move(regex_button));
    addChildStart(std::move(case_sensitive_button));
    addChildStart(std::move(whole_word_button));
    addChildStart(std::move(wrap_button));
    addChildStart(std::move(in_selection_button));
    addChildStart(std::move(highlight_matches_button));

    auto panel_close_button =
        std::make_unique<ImageButtonWidget>(panel_close_image_id, kCloseIconColor, Rgba{}, 0);
    panel_close_button->setAutoresizing(false);
    addChildEnd(std::move(panel_close_button));

    auto find_all_button = std::make_unique<TextButtonWidget>(
        ui_font_id, "Find All", kIconBackgroundColor, app::Size{20, 8}, app::Size{200, 52});
    auto find_prev_button = std::make_unique<TextButtonWidget>(
        ui_font_id, "Find Prev", kIconBackgroundColor, app::Size{20, 8}, app::Size{200, 52});
    auto find_button = std::make_unique<TextButtonWidget>(ui_font_id, "Find", kIconBackgroundColor,
                                                          app::Size{20, 8}, app::Size{200, 52});
    find_all_button->setAutoresizing(false);
    find_prev_button->setAutoresizing(false);
    find_button->setAutoresizing(false);
    int button_height = find_button->getHeight();
    addChildEnd(std::move(find_all_button));
    addChildEnd(std::move(find_prev_button));
    addChildEnd(std::move(find_button));

    int text_input_height = text_input_widget->getHeight();
    setMainWidget(std::unique_ptr<TextInputWidget>(text_input_widget));

    // Calculate max height. We assume all buttons have the same height.
    text_input_height += kVerticalPadding * 2;  // Pad top and bottom.
    button_height += kVerticalPadding * 2;      // Pad top and bottom.
    setHeight(std::max(text_input_height, button_height));
    setMinimumHeight(getHeight());
}

void FindPanelWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();
    rect_renderer.addRect(position, size, kFindPanelColor, Layer::kTwo);

    HorizontalLayoutWidget::draw();
}

}  // namespace gui
