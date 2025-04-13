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
    regex_button->set_autoresizing(false);
    case_sensitive_button->set_autoresizing(false);
    whole_word_button->set_autoresizing(false);
    wrap_button->set_autoresizing(false);
    in_selection_button->set_autoresizing(false);
    highlight_matches_button->set_autoresizing(false);
    addChildStart(std::move(regex_button));
    addChildStart(std::move(case_sensitive_button));
    addChildStart(std::move(whole_word_button));
    addChildStart(std::move(wrap_button));
    addChildStart(std::move(in_selection_button));
    addChildStart(std::move(highlight_matches_button));

    auto panel_close_button =
        std::make_unique<ImageButtonWidget>(panel_close_image_id, kCloseIconColor, Rgb{}, 0);
    panel_close_button->set_autoresizing(false);
    addChildEnd(std::move(panel_close_button));

    auto find_all_button = std::make_unique<TextButtonWidget>(
        ui_font_id, "Find All", kIconBackgroundColor, Size{20, 8}, Size{200, 52});
    auto find_prev_button = std::make_unique<TextButtonWidget>(
        ui_font_id, "Find Prev", kIconBackgroundColor, Size{20, 8}, Size{200, 52});
    auto find_button = std::make_unique<TextButtonWidget>(ui_font_id, "Find", kIconBackgroundColor,
                                                          Size{20, 8}, Size{200, 52});
    find_all_button->set_autoresizing(false);
    find_prev_button->set_autoresizing(false);
    find_button->set_autoresizing(false);
    int button_height = find_button->height();
    addChildEnd(std::move(find_all_button));
    addChildEnd(std::move(find_prev_button));
    addChildEnd(std::move(find_button));

    int text_input_height = text_input_widget->height();
    setMainWidget(std::unique_ptr<TextInputWidget>(text_input_widget));

    // Calculate max height. We assume all buttons have the same height.
    text_input_height += kVerticalPadding * 2;  // Pad top and bottom.
    button_height += kVerticalPadding * 2;      // Pad top and bottom.
    set_height(std::max(text_input_height, button_height));
    set_min_height(height());
}

void FindPanelWidget::draw() {
    auto& rect_renderer = Renderer::instance().rect_renderer();
    rect_renderer.add_rect(position(), size(), position(), position() + size(), kFindPanelColor,
                           Layer::kBackground);

    HorizontalLayoutWidget::draw();
}

}  // namespace gui
