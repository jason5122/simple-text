#include "gui/renderer/renderer.h"
#include "label_widget.h"

// TODO: Debug use; remove this.
#include "util/profile_util.h"

namespace gui {

LabelWidget::LabelWidget(const Size& size, int left_padding, int right_padding)
    : Widget{size}, left_padding{left_padding}, right_padding{right_padding} {}

void LabelWidget::setText(std::string_view str8, const Rgb& color) {
    const auto& glyph_cache = Renderer::instance().getGlyphCache();
    const auto& font_rasterizer = font::FontRasterizer::instance();

    layout = font_rasterizer.layoutLine(glyph_cache.uiFontId(), str8);
    this->color = color;
}

void LabelWidget::addLeftIcon(size_t icon_id) {
    left_side_icons.emplace_back(icon_id);
}

void LabelWidget::addRightIcon(size_t icon_id) {
    right_side_icons.emplace_back(icon_id);
}

void LabelWidget::draw(const std::optional<Point>& mouse_pos) {
    TextRenderer& text_renderer = Renderer::instance().getTextRenderer();
    ImageRenderer& image_renderer = Renderer::instance().getImageRenderer();

    // RectRenderer& rect_renderer = Renderer::instance().getRectRenderer();
    // rect_renderer.addRect(position, size, kTempColor);

    // Draw all left side icons.
    Point left_offset{.x = left_padding};
    for (size_t icon_id : left_side_icons) {
        Size image_size = image_renderer.getImageSize(icon_id);

        Point icon_position = centerVertically(image_size.height) + left_offset;
        image_renderer.addImage(icon_id, icon_position, kFolderIconColor);

        left_offset.x += image_size.width;
    }

    // Draw all right side icons.
    Point right_offset{.x = right_padding};
    for (size_t icon_id : right_side_icons) {
        Size image_size = image_renderer.getImageSize(icon_id);

        right_offset.x += image_size.width;

        Point icon_position = centerVertically(image_size.height) - right_offset;
        icon_position += Point{size.width, 0};
        image_renderer.addImage(icon_id, icon_position, kFolderIconColor);
    }

    const auto& glyph_cache = Renderer::instance().getGlyphCache();
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.getMetrics(glyph_cache.uiFontId());

    Point coords = centerVertically(metrics.line_height) + left_offset;
    int min_x = 0;
    int max_x = size.width - left_padding - right_padding;
    const auto highlight_callback = [this](size_t) { return color; };
    text_renderer.renderLineLayout(layout, coords, TextRenderer::TextLayer::kBackground,
                                   highlight_callback, min_x, max_x);
}

Point LabelWidget::centerVertically(int widget_height) {
    Point centered_point = position;
    centered_point.y += size.height / 2;
    centered_point.y -= widget_height / 2;
    return centered_point;
}

}
