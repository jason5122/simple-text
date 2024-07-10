#include "gui/renderer/renderer.h"
#include "tab_bar_widget.h"

namespace gui {

TabBarWidget::TabBarWidget(const Size& size) : Widget{size} {
    tab_index = 2;

    for (size_t i = 0; i < 3; i++) {
        Size label_pos{
            .width = tab_width - tab_corner_radius * 2,
            .height = size.height,
        };
        std::unique_ptr<LabelWidget> tab_name_label{new LabelWidget{label_pos}};
        tab_name_label->setText(std::format("tab_{}", i));
        tab_name_label->addRightIcon(ImageRenderer::kPanelClose2xIndex);
        tab_name_labels.push_back(std::move(tab_name_label));
    }
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
    tab_pos.x += (tab_width - tab_corner_radius * 2) * tab_index;
    rect_renderer.addRect(tab_pos, {tab_width, size.height}, tab_color, 0, tab_corner_radius);

    Point tab_separator_pos = position;
    tab_separator_pos.x += (tab_width - tab_corner_radius) * 1;  // TODO: Don't hard code 1.
    tab_separator_pos.x -= tab_separator_size.width;

    // Center tab separator.
    tab_separator_pos.y += size.height / 2;
    tab_separator_pos.y -= tab_separator_size.height / 2;

    rect_renderer.addRect(tab_separator_pos, tab_separator_size, tab_separator_color);
}

void TabBarWidget::layout() {
    Point left_width_offset{};
    for (const auto& tab_name_label : tab_name_labels) {
        Point label_pos = position;
        label_pos += Point{tab_corner_radius, 0};
        label_pos += left_width_offset;
        tab_name_label->setPosition(label_pos);

        left_width_offset.x += tab_width - tab_corner_radius * 2;
    }
}

}
