#include "base/numeric/saturation_arithmetic.h"
#include "gui/renderer/renderer.h"
#include "gui/widget/tab_bar_widget.h"

namespace gui {

TabBarWidget::TabBarWidget(size_t font_id, int height, size_t panel_close_image_id)
    : Widget{{.height = height}}, font_id_(font_id), panel_close_image_id_(panel_close_image_id) {
    add_tab("untitled");
}

void TabBarWidget::set_index(size_t index) { index_ = index; }

void TabBarWidget::prev_index() { index_ = (index_ + labels_.size() - 1) % labels_.size(); }

void TabBarWidget::next_index() { index_ = (index_ + 1) % labels_.size(); }

void TabBarWidget::last_index() { index_ = base::sub_sat(labels_.size(), size_t{1}); }

void TabBarWidget::add_tab(std::string_view title) {
    Size label_size = {
        .width = kTabWidth - kTabCornerRadius * 2,
        .height = height(),
    };
    auto label = std::make_unique<TabBarLabelWidget>(font_id_, label_size, 22, 16);
    label->set_text(title);
    label->set_color(kTabTextColor);
    label->add_right_icon(panel_close_image_id_);
    labels_.emplace_back(std::move(label));
}

void TabBarWidget::remove_tab(size_t index) {
    if (labels_.empty()) return;

    labels_.erase(labels_.begin() + index);
    index_ = std::clamp(index, size_t{0}, base::sub_sat(labels_.size(), size_t{1}));
}

void TabBarWidget::draw() {
    auto& rect_renderer = Renderer::instance().rect_renderer();

    rect_renderer.add_rect(position(), size(), position(), position() + size(), kTabBarColor,
                           Layer::kBackground);

    for (const auto& tab_name_label : labels_) {
        tab_name_label->draw();
    }

    Point tab_pos = position();
    tab_pos.x += (kTabWidth - kTabCornerRadius * 2) * index_;
    Size tab_size = {kTabWidth, height()};
    rect_renderer.add_rect(tab_pos, tab_size, position(), position() + size(), kTabColor,
                           Layer::kBackground, 0, kTabCornerRadius);

    size_t num_labels = labels_.size();
    for (size_t i = 0; i < num_labels; ++i) {
        if (i == index_ || i == base::sub_sat(index_, size_t{1})) {
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
    for (const auto& tab_name_label : labels_) {
        Point label_pos = position();
        label_pos += Point{kTabCornerRadius, 0};
        label_pos += left_width_offset;
        tab_name_label->set_position(label_pos);

        left_width_offset.x += kTabWidth - kTabCornerRadius * 2;
    }
}

}  // namespace gui
