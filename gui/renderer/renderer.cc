#include "renderer.h"

#include "opengl/gl.h"
using namespace opengl;

namespace gui {

static_assert(!std::is_copy_constructible_v<Renderer>);
static_assert(!std::is_copy_assignable_v<Renderer>);
static_assert(std::is_move_constructible_v<Renderer>);
static_assert(std::is_move_assignable_v<Renderer>);

Renderer::Renderer() {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    // TODO: Don't hard code the color here.
    // glClearColor(253.0f / 255, 253.0f / 255, 253.0f / 255, 1.0f);  // Light.
    glClearColor(48.0f / 255, 56.0f / 255, 65.0f / 255, 1.0f);  // Dark.
}

Renderer& Renderer::instance() {
    static Renderer renderer;
    return renderer;
}

TextureCache& Renderer::getTextureCache() {
    return texture_cache;
}

LineLayoutCache& Renderer::getLineLayoutCache() {
    return line_layout_cache;
}

TextureRenderer& Renderer::getTextureRenderer() {
    return texture_renderer;
}

RectRenderer& Renderer::getRectRenderer() {
    return rect_renderer;
}

SelectionRenderer& Renderer::getSelectionRenderer() {
    return selection_renderer;
}

void Renderer::flush(const Size& size) {
    glViewport(0, 0, size.width, size.height);
    glClear(GL_COLOR_BUFFER_BIT);

    rect_renderer.flush(size, Layer::kBackground);
    selection_renderer.flush(size);
    texture_renderer.flush(size);
    rect_renderer.flush(size, Layer::kForeground);
}

}  // namespace gui
