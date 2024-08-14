#pragma once

#include "gui/renderer/glyph_cache.h"
#include "gui/renderer/image_renderer.h"
#include "gui/renderer/rect_renderer.h"
#include "gui/renderer/selection_renderer.h"
#include "gui/renderer/text_renderer.h"

namespace gui {

class Renderer {
public:
    static Renderer& instance();

    GlyphCache& getGlyphCache();
    TextRenderer& getTextRenderer();
    RectRenderer& getRectRenderer();
    SelectionRenderer& getSelectionRenderer();
    ImageRenderer& getImageRenderer();

    void flush(const Size& size);

private:
    Renderer();

    GlyphCache glyph_cache;
    TextRenderer text_renderer;
    RectRenderer rect_renderer;
    SelectionRenderer selection_renderer;
    ImageRenderer image_renderer;

    Point editor_offset{200 * 2, 30 * 2};
};

}
