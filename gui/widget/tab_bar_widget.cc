#include "base/numeric/literals.h"
#include "base/numeric/saturation_arithmetic.h"
#include "base/numeric/wrap_arithmetic.h"
#include "gui/renderer/renderer.h"
#include "tab_bar_widget.h"

namespace gui {

TabBarWidget::TabBarWidget(size_t font_id, int height, size_t panel_close_image_id)
    : Widget{{.height = height}}, font_id(font_id), panel_close_image_id(panel_close_image_id) {
    addTab("untitled");
}

void TabBarWidget::setIndex(size_t index) {
    if (index < tab_name_labels.size()) {
        this->index = index;
    }
}

void TabBarWidget::prevIndex() {
    if (index >= tab_name_labels.size()) return;
    index = base::dec_wrap(index, tab_name_labels.size());
}

void TabBarWidget::nextIndex() {
    if (index >= tab_name_labels.size()) return;
    index = base::inc_wrap(index, tab_name_labels.size());
}

void TabBarWidget::lastIndex() {
    index = base::sub_sat(tab_name_labels.size(), 1_Z);
}

void TabBarWidget::addTab(std::string_view title) {
    app::Size label_size{
        .width = kTabWidth - kTabCornerRadius * 2,
        .height = size.height,
    };
    std::unique_ptr<LabelWidget> tab_name_label{new LabelWidget(font_id, label_size, 22, 16)};
    tab_name_label->setText(title);
    tab_name_label->setColor(kTabTextColor);
    tab_name_label->addRightIcon(panel_close_image_id);
    tab_name_labels.emplace_back(std::move(tab_name_label));
}

void TabBarWidget::removeTab(size_t index) {
    if (tab_name_labels.empty()) return;

    tab_name_labels.erase(tab_name_labels.begin() + index);
    this->index = std::clamp(index, 0_Z, base::sub_sat(tab_name_labels.size(), 1_Z));
}

void TabBarWidget::draw() {
    auto& rect_renderer = Renderer::instance().getRectRenderer();

    rect_renderer.addRect(position, size, kTabBarColor, RectRenderer::RectLayer::kForeground);

    for (const auto& tab_name_label : tab_name_labels) {
        tab_name_label->draw();
    }

    app::Point tab_pos = position;
    tab_pos.x += (kTabWidth - kTabCornerRadius * 2) * index;
    rect_renderer.addRect(tab_pos, {kTabWidth, size.height}, kTabColor,
                          RectRenderer::RectLayer::kForeground, 0, kTabCornerRadius);

    size_t num_labels = tab_name_labels.size();
    for (size_t i = 0; i < num_labels; ++i) {
        if (i == index || i == base::sub_sat(index, 1_Z)) {
            continue;
        }
        if (i == num_labels - 1) {
            continue;
        }

        app::Point tab_separator_pos = position;
        tab_separator_pos.x -= kTabSeparatorSize.width;
        tab_separator_pos.x += (kTabWidth - kTabCornerRadius * 2) * (i + 1) + kTabCornerRadius;

        // Center tab separator.
        tab_separator_pos.y += size.height / 2;
        tab_separator_pos.y -= kTabSeparatorSize.height / 2;

        rect_renderer.addRect(tab_separator_pos, kTabSeparatorSize, kTabSeparatorColor,
                              RectRenderer::RectLayer::kForeground);
    }
}

void TabBarWidget::layout() {
    app::Point left_width_offset{};
    for (const auto& tab_name_label : tab_name_labels) {
        app::Point label_pos = position;
        label_pos += app::Point{kTabCornerRadius, 0};
        label_pos += left_width_offset;
        tab_name_label->setPosition(label_pos);

        left_width_offset.x += kTabWidth - kTabCornerRadius * 2;
    }
}

}  // namespace gui
