#pragma once

#include "gui/renderer/line_layout_cache.h"
#include "gui/renderer/rect_renderer.h"
#include "gui/renderer/selection_renderer.h"
#include "gui/renderer/texture_cache.h"
#include "gui/renderer/texture_renderer.h"

namespace gui {

class Renderer {
public:
    inline static Renderer& instance() {
        static Renderer renderer;
        return renderer;
    }

    constexpr TextureCache& texture_cache() { return texture_cache_; }
    constexpr LineLayoutCache& line_layout_cache() { return line_layout_cache_; }

    constexpr TextureRenderer& texture_renderer() { return texture_renderer_; }
    constexpr RectRenderer& rect_renderer() { return rect_renderer_; }
    constexpr SelectionRenderer& selection_renderer() { return selection_renderer_; }

    void flush(const Size& size);

private:
    Renderer();

    TextureCache texture_cache_;
    LineLayoutCache line_layout_cache_;

    TextureRenderer texture_renderer_;
    RectRenderer rect_renderer_;
    SelectionRenderer selection_renderer_;
};

}  // namespace gui
