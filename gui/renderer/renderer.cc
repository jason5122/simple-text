#include "gl/gl.h"
#include "gui/renderer/renderer.h"

using namespace gl;

namespace gui {

Renderer::Renderer() {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    // TODO: Don't hard code the color here.
    // glClearColor(253.0f / 255, 253.0f / 255, 253.0f / 255, 1.0f);  // Light.
    glClearColor(48.0f / 255, 56.0f / 255, 65.0f / 255, 1.0f);  // Dark.
}

void Renderer::flush(const Size& size) {
    glViewport(0, 0, size.width, size.height);
    glClear(GL_COLOR_BUFFER_BIT);

    rect_renderer_.flush(size, Layer::kBackground);
    selection_renderer_.flush(size);
    texture_renderer_.flush(size);
    rect_renderer_.flush(size, Layer::kForeground);
}

}  // namespace gui
