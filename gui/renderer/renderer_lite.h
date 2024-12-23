#pragma once

#include "gui/renderer/line_layout_cache.h"
#include "gui/renderer/rect_renderer.h"
#include "gui/renderer/text_renderer.h"

namespace gui {

class RendererLite {
public:
    static RendererLite& instance();

    LineLayoutCache& getLineLayoutCache();
    TextRenderer& getTextRenderer();
    RectRenderer& getRectRenderer();

    void flush(const app::Size& size);

private:
    RendererLite();

    LineLayoutCache line_layout_cache;  // TODO: Consider making this a member in TextRenderer.
    TextRenderer text_renderer;
    RectRenderer rect_renderer;
};

}  // namespace gui
