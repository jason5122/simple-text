#pragma once

#include "gui/renderer/line_layout_cache.h"
#include "gui/renderer/rect_renderer.h"
#include "gui/renderer/selection_renderer.h"
#include "gui/renderer/texture_renderer.h"

namespace gui {

class Renderer {
public:
    static Renderer& instance();

    TextureCache& getTextureCache();
    LineLayoutCache& getLineLayoutCache();

    TextureRenderer& getTextureRenderer();
    RectRenderer& getRectRenderer();
    SelectionRenderer& getSelectionRenderer();

    void flush(const app::Size& size);

private:
    Renderer();

    TextureCache texture_cache;
    LineLayoutCache line_layout_cache;

    TextureRenderer texture_renderer;
    RectRenderer rect_renderer;
    SelectionRenderer selection_renderer;
};

}  // namespace gui
