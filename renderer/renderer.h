#pragma once

#include "renderer/image_renderer.h"
#include "renderer/movement.h"
#include "renderer/rect_renderer.h"
#include "renderer/selection_renderer.h"
#include "renderer/text/glyph_cache.h"
#include "renderer/text/text_renderer.h"

namespace renderer {

class Renderer {
public:
    static Renderer& instance();

    TextRenderer& getTextRenderer();
    RectRenderer& getRectRenderer();
    SelectionRenderer& getSelectionRenderer();
    ImageRenderer& getImageRenderer();
    Movement& getMovement();

    void flush(const Size& size);

private:
    Renderer();
    ~Renderer() = default;

    GlyphCache main_glyph_cache;
    GlyphCache ui_glyph_cache;
    TextRenderer text_renderer;
    RectRenderer rect_renderer;
    SelectionRenderer selection_renderer;
    ImageRenderer image_renderer;
    Movement movement;

    Point editor_offset{200 * 2, 30 * 2};
};

}
