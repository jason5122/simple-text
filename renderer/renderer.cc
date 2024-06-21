#include "renderer.h"

#include "opengl/gl.h"
using namespace opengl;

namespace renderer {

Renderer* g_renderer = nullptr;

Renderer::Renderer()
    : main_glyph_cache{"Source Code Pro", 16 * 2},
      ui_glyph_cache{"Arial", 11 * 2},
      text_renderer{main_glyph_cache, ui_glyph_cache},
      rect_renderer{},
      selection_renderer{main_glyph_cache},
      movement{main_glyph_cache} {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glClearColor(253.0f / 255, 253.0f / 255, 253.0f / 255, 1.0f);
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

Movement& Renderer::getMovement() {
    return movement;
}

void Renderer::flush(const Size& size) {
    glViewport(0, 0, size.width, size.height);
    glClear(GL_COLOR_BUFFER_BIT);

    selection_renderer.render(size, 0);
    text_renderer.flush(size);
    selection_renderer.render(size, 1);
    rect_renderer.flush(size);

    selection_renderer.destroyInstances();
}

}
