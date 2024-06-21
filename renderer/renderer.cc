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
      selection_renderer{},
      // selection_renderer{main_glyph_cache},
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

    // selection_renderer.render(0);
    // text_renderer.flush(size);
    // selection_renderer.render(1);
    // rect_renderer.flush(size);

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    std::vector<SelectionRenderer::Selection> selections = {{10, 100, 200}};
    selection_renderer.createInstances(size, {0, 0}, {400, 64}, main_glyph_cache, selections, 100);
    selection_renderer.render(0);
    selection_renderer.render(1);

    selection_renderer.destroyInstances();
}

}
