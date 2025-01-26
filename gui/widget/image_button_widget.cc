#include "image_button_widget.h"

#include "gui/renderer/renderer.h"

namespace gui {

ImageButtonWidget::ImageButtonWidget(size_t image_id,
                                     const Rgb& text_color,
                                     const Rgb& bg_color,
                                     int padding)
    : image_id(image_id), text_color(text_color), bg_color(bg_color), padding(padding) {
    const auto& texture_cache = gui::Renderer::instance().getTextureCache();
    auto& image = texture_cache.getImage(image_id);

    auto new_size = image.size;
    new_size += {padding * 2, padding * 2};
    set_size(new_size);
}

void ImageButtonWidget::draw() {
    // TODO: Formalize this.
    if (getState()) {
        auto& rect_renderer = Renderer::instance().getRectRenderer();
        rect_renderer.addRect(position(), size(), position(), position() + size(), bg_color,
                              Layer::kBackground, 4);
    }

    auto pos = position();
    pos += {padding, padding};

    auto& texture_renderer = Renderer::instance().getTextureRenderer();
    texture_renderer.addImage(image_id, pos, text_color);
}

}  // namespace gui
