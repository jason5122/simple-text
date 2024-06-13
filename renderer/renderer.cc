#include "opengl/functionsgl_enums.h"
#include "renderer.h"

namespace renderer {

Renderer::Renderer(opengl::FunctionsGL* gl)
    : gl{gl}, main_glyph_cache{gl, "Source Code Pro", 16 * 2}, ui_glyph_cache{gl, "Arial", 11 * 2},
      text_renderer{gl, main_glyph_cache, ui_glyph_cache}, rect_renderer{gl} {}

void Renderer::setup() {
    main_glyph_cache.setup();
    ui_glyph_cache.setup();
    text_renderer.setup();
    rect_renderer.setup();

    gl->enable(GL_BLEND);
    gl->depthMask(GL_FALSE);

    gl->clearColor(1.0f, 1.0f, 1.0f, 1.0f);
    // gl->clearColor(1.0f, 0.0f, 1.0f, 1.0f);
}

void Renderer::draw(const Size& size, const base::Buffer& buffer) {
    gl->viewport(0, 0, size.width, size.height);

    gl->clear(GL_COLOR_BUFFER_BIT);

    gl->blendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    int longest_line;
    int end_caret_x;
    Point editor_offset = {200 * 2, 30 * 2};
    text_renderer.renderText(size, {0, 0}, buffer, editor_offset, {0, 0, 0}, {0, 0, 0},
                             longest_line, 100, end_caret_x);

    gl->blendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    rect_renderer.draw(size, {0, 0}, {0, 0, 0}, 0, 20 * 2, 50, 1000, editor_offset, 10 * 2);
}

}
