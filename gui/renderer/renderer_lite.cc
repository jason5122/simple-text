#include "renderer_lite.h"

#include "opengl/gl.h"
using namespace opengl;

// TODO: Remove this.
#include "util/profile_util.h"

namespace gui {

RendererLite::RendererLite() {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    // glClearColor(253.0f / 255, 253.0f / 255, 253.0f / 255, 1.0f);  // Light.
    glClearColor(48.0f / 255, 56.0f / 255, 65.0f / 255, 1.0f);  // Dark.
}

RendererLite& RendererLite::instance() {
    static RendererLite renderer;
    return renderer;
}

LineLayoutCache& RendererLite::getLineLayoutCache() {
    return line_layout_cache;
}

TextRenderer& RendererLite::getTextRenderer() {
    return text_renderer;
}

RectRenderer& RendererLite::getRectRenderer() {
    return rect_renderer;
}

void RendererLite::flush(const app::Size& size) {
    glViewport(0, 0, size.width, size.height);
    glClear(GL_COLOR_BUFFER_BIT);

    // TODO: Add arbitrarily many layers (or a fixed but larger set of layers).
    rect_renderer.flush(size, RectRenderer::RectLayer::kBackground);
    text_renderer.flush(size, TextRenderer::TextLayer::kBackground);
    rect_renderer.flush(size, RectRenderer::RectLayer::kForeground);
    text_renderer.flush(size, TextRenderer::TextLayer::kForeground);
}

}  // namespace gui
