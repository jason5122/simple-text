#pragma once

#include "gui/renderer/line_layout_cache.h"
#include "gui/renderer/rect_renderer.h"
#include "gui/renderer/texture_renderer.h"

namespace gui {

class RendererLite {
public:
    static RendererLite& instance();

    LineLayoutCache& getLineLayoutCache();
    TextureRenderer& getTextureRenderer();
    RectRenderer& getRectRenderer();

    void flush(const app::Size& size);

private:
    RendererLite();

    LineLayoutCache line_layout_cache;
    TextureRenderer texture_renderer;
    RectRenderer rect_renderer;
};

}  // namespace gui
