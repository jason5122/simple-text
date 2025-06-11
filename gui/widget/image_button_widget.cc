#include "gui/renderer/renderer.h"
#include "gui/widget/image_button_widget.h"

namespace gui {

ImageButtonWidget::ImageButtonWidget(size_t image_id,
                                     const Rgb& text_color,
                                     const Rgb& bg_color,
                                     int padding)
    : image_id(image_id), text_color(text_color), bg_color(bg_color), padding(padding) {
    const auto& texture_cache = gui::Renderer::instance().texture_cache();
    auto& image = texture_cache.get_image(image_id);

    auto new_size = image.size;
    new_size += {padding * 2, padding * 2};
    set_size(new_size);
}

void ImageButtonWidget::draw() {
    // TODO: Formalize this.
    if (get_state()) {
        auto& rect_renderer = Renderer::instance().rect_renderer();
        rect_renderer.add_rect(position(), size(), position(), position() + size(), bg_color,
                               Layer::kBackground, 4);
    }

    auto pos = position();
    pos += {padding, padding};

    auto& texture_renderer = Renderer::instance().texture_renderer();
    texture_renderer.add_image(image_id, pos, text_color);
}

}  // namespace gui
