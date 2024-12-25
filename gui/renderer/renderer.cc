#include "renderer.h"

#include "opengl/gl.h"
using namespace opengl;

namespace gui {

Renderer::Renderer() {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    // glClearColor(253.0f / 255, 253.0f / 255, 253.0f / 255, 1.0f);  // Light.
    glClearColor(48.0f / 255, 56.0f / 255, 65.0f / 255, 1.0f);  // Dark.
}

Renderer& Renderer::instance() {
    static Renderer renderer;
    return renderer;
}

LineLayoutCache& Renderer::getLineLayoutCache() {
    return line_layout_cache;
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

void Renderer::flush(const app::Size& size) {
    glViewport(0, 0, size.width, size.height);
    glClear(GL_COLOR_BUFFER_BIT);

    // TODO: Add arbitrarily many layers (or a fixed but larger set of layers).

    rect_renderer.flush(size, Layer::kOne);
    selection_renderer.flush(size, Layer::kOne);
    text_renderer.flush(size, Layer::kOne);
    image_renderer.renderBatch(size, Layer::kOne);

    rect_renderer.flush(size, Layer::kTwo);
    selection_renderer.flush(size, Layer::kTwo);
    text_renderer.flush(size, Layer::kTwo);
    image_renderer.renderBatch(size, Layer::kTwo);
}

}  // namespace gui
