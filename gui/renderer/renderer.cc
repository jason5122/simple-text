#include "renderer.h"

#include "opengl/gl.h"
using namespace opengl;

namespace gui {

Renderer::Renderer() : text_renderer{glyph_cache}, selection_renderer{glyph_cache} {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glClearColor(253.0f / 255, 253.0f / 255, 253.0f / 255, 1.0f);
}

Renderer& Renderer::instance() {
    static Renderer renderer;
    return renderer;
}

GlyphCache& Renderer::getGlyphCache() {
    return glyph_cache;
}

TextRenderer& Renderer::getTextRenderer() {
    return text_renderer;
}

RectRenderer& Renderer::getRectRenderer() {
    return rect_renderer;
}

SelectionRenderer& Renderer::getSelectionRenderer() {
    return selection_renderer;
}

ImageRenderer& Renderer::getImageRenderer() {
    return image_renderer;
}

void Renderer::flush(const Size& size) {
    glViewport(0, 0, size.width, size.height);
    glClear(GL_COLOR_BUFFER_BIT);

    selection_renderer.flush(size);

    // TODO: Add arbitrarily many layers (or a fixed but larger set of layers).
    rect_renderer.flush(size, RectRenderer::RectLayer::kBackground);
    text_renderer.flush(size, TextRenderer::TextLayer::kBackground);
    rect_renderer.flush(size, RectRenderer::RectLayer::kForeground);
    text_renderer.flush(size, TextRenderer::TextLayer::kForeground);

    image_renderer.flush(size);
}

}
