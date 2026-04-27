#pragma once

#include "gfx/frame.h"
#include "ui/types.h"

namespace ui {

class PaintContext {
public:
    explicit PaintContext(gfx::Frame& frame) : frame_(frame) {}

    void fill_rect(Rect rect, Color color);

private:
    gfx::Frame& frame_;
};

}  // namespace ui
