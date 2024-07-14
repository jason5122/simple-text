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

static inline int PositiveModulo(int i, int n) {
    return (i % n + n) % n;
}

void TabBarWidget::prevIndex() {
    index = PositiveModulo(index - 1, tab_name_labels.size());
}

void TabBarWidget::nextIndex() {
    index = PositiveModulo(index + 1, tab_name_labels.size());
}

void TabBarWidget::addTab(const std::string& title) {
    Size label_size{
        .width = kTabWidth - kTabCornerRadius * 2,
        .height = size.height,
    };
    std::unique_ptr<LabelWidget> tab_name_label{new LabelWidget{label_size, 22, 16}};
    tab_name_label->setText(title, {92, 92, 92});
    tab_name_label->addRightIcon(ImageRenderer::kPanelClose2xIndex);
    tab_name_labels.push_back(std::move(tab_name_label));
}

void TabBarWidget::removeTab(size_t index) {
    tab_name_labels.erase(tab_name_labels.begin() + index);
}

void TabBarWidget::draw() {
    RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();
    ImageRenderer& image_renderer = Renderer::instance().getImageRenderer();

    constexpr Rgba tab_bar_color{190, 190, 190, 255};
    constexpr Rgba tab_color{253, 253, 253, 255};
    constexpr Rgba tab_separator_color{148, 149, 149, 255};

    rect_renderer.addRect(position, size, tab_bar_color);

    for (const auto& tab_name_label : tab_name_labels) {
        tab_name_label->draw();
    }

    Point tab_pos = position;
    tab_pos.x += (kTabWidth - kTabCornerRadius * 2) * index;
    rect_renderer.addRect(tab_pos, {kTabWidth, size.height}, tab_color, 0, kTabCornerRadius);

    Point tab_separator_pos = position;
    tab_separator_pos.x += (kTabWidth - kTabCornerRadius) * 1;  // TODO: Don't hard code 1.
    tab_separator_pos.x -= kTabSeparatorSize.width;

    // Center tab separator.
    tab_separator_pos.y += size.height / 2;
    tab_separator_pos.y -= kTabSeparatorSize.height / 2;

    rect_renderer.addRect(tab_separator_pos, kTabSeparatorSize, tab_separator_color);
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
