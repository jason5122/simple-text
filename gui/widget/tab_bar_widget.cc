#include "tab_bar_widget.h"

#include "base/numeric/literals.h"
#include "base/numeric/saturation_arithmetic.h"
#include "base/numeric/wrap_arithmetic.h"
#include "gui/renderer/renderer.h"

namespace gui {

TabBarWidget::TabBarWidget(size_t font_id, int height, size_t panel_close_image_id)
    : Widget{{.height = height}}, font_id(font_id), panel_close_image_id(panel_close_image_id) {
    add_tab("untitled");
}

void TabBarWidget::set_index(size_t index) {
    if (index < tab_name_labels.size()) {
        this->index = index;
    }
}

void TabBarWidget::prev_index() {
    if (index >= tab_name_labels.size()) return;
    index = base::dec_wrap(index, tab_name_labels.size());
}

void TabBarWidget::next_index() {
    if (index >= tab_name_labels.size()) return;
    index = base::inc_wrap(index, tab_name_labels.size());
}

void TabBarWidget::last_index() { index = base::sub_sat(tab_name_labels.size(), 1_Z); }

void TabBarWidget::add_tab(std::string_view title) {
    Size label_size = {
        .width = kTabWidth - kTabCornerRadius * 2,
        .height = height(),
    };
    auto tab_name_label = std::make_unique<TabBarLabelWidget>(font_id, label_size, 22, 16);
    tab_name_label->set_text(title);
    tab_name_label->set_color(kTabTextColor);
    tab_name_label->add_right_icon(panel_close_image_id);
    tab_name_labels.emplace_back(std::move(tab_name_label));
}

void TabBarWidget::remove_tab(size_t index) {
    if (tab_name_labels.empty()) return;

    tab_name_labels.erase(tab_name_labels.begin() + index);
    this->index = std::clamp(index, 0_Z, base::sub_sat(tab_name_labels.size(), 1_Z));
}

void TabBarWidget::draw() {
    auto& rect_renderer = Renderer::instance().rect_renderer();

    rect_renderer.add_rect(position(), size(), position(), position() + size(), kTabBarColor,
                           Layer::kBackground);

    for (const auto& tab_name_label : tab_name_labels) {
        tab_name_label->draw();
    }

    Point tab_pos = position();
    tab_pos.x += (kTabWidth - kTabCornerRadius * 2) * index;
    Size tab_size = {kTabWidth, height()};
    rect_renderer.add_rect(tab_pos, tab_size, position(), position() + size(), kTabColor,
                           Layer::kBackground, 0, kTabCornerRadius);

    size_t num_labels = tab_name_labels.size();
    for (size_t i = 0; i < num_labels; ++i) {
        if (i == index || i == base::sub_sat(index, 1_Z)) {
            continue;
        }
        if (i == num_labels - 1) {
            continue;
        }

        Point tab_separator_pos = position();
        tab_separator_pos.x -= kTabSeparatorSize.width;
        tab_separator_pos.x += (kTabWidth - kTabCornerRadius * 2) * (i + 1) + kTabCornerRadius;

        // Center tab separator.
        tab_separator_pos.y += height() / 2;
        tab_separator_pos.y -= kTabSeparatorSize.height / 2;

        rect_renderer.add_rect(tab_separator_pos, kTabSeparatorSize, position(),
                               position() + size(), kTabSeparatorColor, Layer::kBackground);
    }
}

void TabBarWidget::layout() {
    Point left_width_offset{};
    for (const auto& tab_name_label : tab_name_labels) {
        Point label_pos = position();
        label_pos += Point{kTabCornerRadius, 0};
        label_pos += left_width_offset;
        tab_name_label->set_position(label_pos);

        left_width_offset.x += kTabWidth - kTabCornerRadius * 2;
    }
}

}  // namespace gui
