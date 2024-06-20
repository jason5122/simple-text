#pragma once

#include "renderer/movement.h"
#include "renderer/rect_renderer.h"
#include "renderer/text/glyph_cache.h"
#include "renderer/text/text_renderer.h"

namespace renderer {

class Renderer {
public:
    Renderer();

    TextRenderer& getTextRenderer();
    RectRenderer& getRectRenderer();
    Movement& getMovement();

    void draw(const Size& size,
              const base::Buffer& buffer,
              const Point& scroll_offset,
              const CaretInfo& end_caret);
    void flush(const Size& size);

private:
    GlyphCache main_glyph_cache;
    GlyphCache ui_glyph_cache;
    TextRenderer text_renderer;
    RectRenderer rect_renderer;
    Movement movement;

    Point editor_offset{200 * 2, 30 * 2};
};

}
