#include "renderer/renderer.h"
#include "tab_bar_widget.h"

namespace gui {

TabBarWidget::TabBarWidget(const renderer::Size& size) : Widget{size} {
    tab_index = 2;

    for (size_t i = 0; i < 3; i++) {
        renderer::Size label_pos{
            .width = tab_width - tab_corner_radius * 2,
            .height = size.height,
        };
        std::unique_ptr<LabelWidget> tab_name_label{new LabelWidget{label_pos}};
        tab_name_label->setText(std::format("tab_{}", i));
        tab_name_label->addRightIcon(renderer::ImageRenderer::kPanelClose2xIndex);
        tab_name_labels.push_back(std::move(tab_name_label));
    }
}

void TabBarWidget::draw() {
    renderer::RectRenderer& rect_renderer = renderer::Renderer::instance().getRectRenderer();
    renderer::ImageRenderer& image_renderer = renderer::Renderer::instance().getImageRenderer();

    constexpr renderer::Rgba tab_bar_color{190, 190, 190, 255};
    constexpr renderer::Rgba tab_color{253, 253, 253, 255};
    constexpr renderer::Rgba tab_separator_color{148, 149, 149, 255};

    rect_renderer.addRect(position, size, tab_bar_color);

    for (const auto& tab_name_label : tab_name_labels) {
        tab_name_label->draw();
    }

    rect_renderer.addRect(position + renderer::Point{340 * tab_index, 0}, {tab_width, size.height},
                          tab_color, 0, tab_corner_radius);

    // TODO: Figure out why we need to add 1 to `26 / 2`.
    rect_renderer.addRect(position + renderer::Point{350 * 1 - 2, 26 / 2 + 1},
                          {2, size.height - 26}, tab_separator_color);
}

void TabBarWidget::layout() {
    renderer::Point left_width_offset{};
    for (const auto& tab_name_label : tab_name_labels) {
        renderer::Point label_pos = position;
        label_pos += renderer::Point{tab_corner_radius, 0};
        label_pos += left_width_offset;
        tab_name_label->setPosition(label_pos);

        left_width_offset.x += tab_width - tab_corner_radius * 2;
    }
}

}
