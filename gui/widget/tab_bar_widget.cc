#include "base/numeric/literals.h"
#include "base/numeric/saturation_arithmetic.h"
#include "base/numeric/wrap_arithmetic.h"
#include "gui/renderer/renderer.h"
#include "tab_bar_widget.h"

namespace gui {

TabBarWidget::TabBarWidget(int height) : Widget{{.height = height}} {
    addTab("untitled");
}

void TabBarWidget::setIndex(size_t index) {
    if (index < tab_name_labels.size()) {
        this->index = index;
    }
}

void TabBarWidget::prevIndex() {
    index = base::dec_wrap(index, tab_name_labels.size());
}

void TabBarWidget::nextIndex() {
    index = base::inc_wrap(index, tab_name_labels.size());
}

void TabBarWidget::lastIndex() {
    index = base::sub_sat(tab_name_labels.size(), 1_Z);
}

void TabBarWidget::addTab(std::string_view title) {
    Size label_size{
        .width = kTabWidth - kTabCornerRadius * 2,
        .height = size.height,
    };
    std::unique_ptr<LabelWidget> tab_name_label{new LabelWidget{label_size, 22, 16}};
    tab_name_label->setText(title, kTabTextColor);
    tab_name_label->addRightIcon(ImageRenderer::kPanelClose2xIndex);
    tab_name_labels.push_back(std::move(tab_name_label));
}

void TabBarWidget::removeTab(size_t index) {
    tab_name_labels.erase(tab_name_labels.begin() + index);
}

void TabBarWidget::draw(const std::optional<Point>& mouse_pos) {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();

    rect_renderer.addRect(position, size, kTabBarColor, RectRenderer::RectLayer::kForeground);

    for (const auto& tab_name_label : tab_name_labels) {
        tab_name_label->draw(mouse_pos);
    }

    Point tab_pos = position;
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

        Point tab_separator_pos = position;
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
    Point left_width_offset{};
    for (const auto& tab_name_label : tab_name_labels) {
        Point label_pos = position;
        label_pos += Point{kTabCornerRadius, 0};
        label_pos += left_width_offset;
        tab_name_label->setPosition(label_pos);

        left_width_offset.x += kTabWidth - kTabCornerRadius * 2;
    }
}

}
