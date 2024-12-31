#pragma once

#include "gui/renderer/image_renderer.h"
#include "gui/renderer/line_layout_cache.h"
#include "gui/renderer/rect_renderer.h"
#include "gui/renderer/selection_renderer.h"
#include "gui/renderer/text_renderer.h"

namespace gui {

class Renderer {
public:
    static Renderer& instance();

    GlyphCache& getGlyphCache();
    LineLayoutCache& getLineLayoutCache();

    TextRenderer& getTextRenderer();
    RectRenderer& getRectRenderer();
    SelectionRenderer& getSelectionRenderer();
    ImageRenderer& getImageRenderer();

    void flush(const app::Size& size);

private:
    Renderer();

    GlyphCache glyph_cache;
    LineLayoutCache line_layout_cache;

    TextRenderer text_renderer;
    RectRenderer rect_renderer;
    SelectionRenderer selection_renderer;
    ImageRenderer image_renderer;
};

}  // namespace gui
