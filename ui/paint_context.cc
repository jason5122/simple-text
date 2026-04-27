#include "ui/paint_context.h"
#include <array>

namespace ui {

void PaintContext::fill_rect(Rect rect, Color color) {
    std::array<gfx::Quad, 1> quad{
        gfx::Quad{rect.x, rect.y, rect.width, rect.height, color.r, color.g, color.b, color.a},
    };
    frame_.draw_quads(quad, 0, 0);
}

}  // namespace ui
