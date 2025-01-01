#include "custom_widget.h"

#include "base/numeric/literals.h"
#include "base/numeric/saturation_arithmetic.h"
#include "gui/renderer/renderer.h"

#include <cmath>
#include <fmt/base.h>
#include <fmt/format.h>

// TODO: Debug use; remove this.
#include "util/profile_util.h"
#include <cassert>

namespace gui {

CustomWidget::CustomWidget(std::string_view text, size_t font_id) : font_id(font_id), tree(text) {
    updateMaxScroll();
}

void CustomWidget::draw() {
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);

    // Calculate start and end lines.
    int main_line_height = metrics.line_height;
    size_t visible_lines = std::ceil(static_cast<double>(size.height) / main_line_height);

    size_t start_line [[maybe_unused]] = scroll_offset.y / main_line_height;
    size_t end_line [[maybe_unused]] = start_line + visible_lines;

    renderText(main_line_height, start_line, end_line);
}

void CustomWidget::leftMouseDown(const app::Point& mouse_pos,
                                 app::ModifierKey modifiers,
                                 app::ClickType click_type) {}

void CustomWidget::leftMouseDrag(const app::Point& mouse_pos,
                                 app::ModifierKey modifiers,
                                 app::ClickType click_type) {}

void CustomWidget::updateMaxScroll() {
    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);

    // TODO: Figure out how to update max width.
    max_scroll_offset.x = 0;
    max_scroll_offset.y = tree.line_count() * metrics.line_height;
}

size_t CustomWidget::lineAtY(int y) const {
    if (y < 0) {
        y = 0;
    }

    const auto& font_rasterizer = font::FontRasterizer::instance();
    const auto& metrics = font_rasterizer.metrics(font_id);

    size_t line = y / metrics.line_height;
    return std::clamp(line, 0_Z, tree.line_count() - 1);
}

inline const font::LineLayout& CustomWidget::layoutAt(size_t line) {
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    std::string line_str = tree.get_line_content_for_layout_use(line);
    return line_layout_cache.get(font_id, line_str);
}

inline constexpr app::Point CustomWidget::textOffset() {
    app::Point text_offset = position - scroll_offset;
    text_offset.x += gutterWidth();
    return text_offset;
}

inline constexpr int CustomWidget::gutterWidth() {
    return kGutterLeftPadding + lineNumberWidth() + kGutterRightPadding;
}

inline int CustomWidget::lineNumberWidth() {
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    int digit_width = line_layout_cache.get(font_id, "0").width;
    int log = std::log10(tree.line_count());
    return digit_width * std::max(log + 1, 2);
}

void CustomWidget::renderText(int main_line_height, size_t start_line, size_t end_line) {
    // Render two lines before start and one line after end. This ensures no sudden cutoff of
    // rendered text.
    start_line = base::sub_sat(start_line, 2_Z);
    end_line = base::add_sat(end_line, 1_Z);

    start_line = std::clamp(start_line, 0_Z, tree.line_count());
    end_line = std::clamp(end_line, 0_Z, tree.line_count());

    auto& texture_renderer = Renderer::instance().getTextureRenderer();
    // auto& rect_renderer = Renderer::instance().getRectRenderer();
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();

    PROFILE_BLOCK("CustomWidget::renderText()");

    for (size_t line = start_line; line < end_line; ++line) {
        const auto& layout = layoutAt(line);

        app::Point coords = textOffset();
        coords.y += static_cast<int>(line) * main_line_height;
        coords.x += kCaretWidth / 2;  // Match Sublime Text.

        int min_x = scroll_offset.x;
        int max_x = scroll_offset.x + size.width;

        texture_renderer.insertLineLayout(
            layout, coords, [](size_t) { return kTextColor; }, min_x, max_x);

        // Draw line numbers.
        app::Point line_number_coords = position;
        line_number_coords.y -= scroll_offset.y;
        line_number_coords.x += kGutterLeftPadding;
        line_number_coords.y += static_cast<int>(line) * main_line_height;

        std::string line_number_str = fmt::format("{}", line + 1);
        const auto& line_number_layout = line_layout_cache.get(font_id, line_number_str);
        line_number_coords.x += lineNumberWidth() - line_number_layout.width;

        const auto line_number_highlight_callback = [](size_t) { return kLineNumberColor; };
        texture_renderer.insertLineLayout(line_number_layout, line_number_coords,
                                          line_number_highlight_callback);
    }
}

}  // namespace gui
