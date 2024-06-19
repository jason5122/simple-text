#include "opengl/functions_gl_enums.h"
#include "renderer.h"

namespace renderer {

Renderer::Renderer(std::shared_ptr<opengl::FunctionsGL> shared_gl)
    : gl{std::move(shared_gl)},
      main_glyph_cache{gl, "Source Code Pro", 16 * 2},
      ui_glyph_cache{gl, "Arial", 11 * 2},
      text_renderer{gl, main_glyph_cache, ui_glyph_cache},
      rect_renderer{gl},
      movement{main_glyph_cache} {
    gl->enable(GL_BLEND);
    gl->depthMask(GL_FALSE);

    gl->clearColor(253.0f / 255, 253.0f / 255, 253.0f / 255, 1.0f);
}

TextRenderer& Renderer::getTextRenderer() {
    return text_renderer;
}

RectRenderer& Renderer::getRectRenderer() {
    return rect_renderer;
}

Movement& Renderer::getMovement() {
    return movement;
}

void Renderer::draw(const Size& size,
                    const base::Buffer& buffer,
                    const Point& scroll_offset,
                    const CaretInfo& end_caret) {
    gl->viewport(0, 0, size.width, size.height);

    gl->clear(GL_COLOR_BUFFER_BIT);

    int longest_line = 0;
    Point end_caret_pos{};

    gl->blendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    text_renderer.renderText(size, scroll_offset, buffer, editor_offset, end_caret, end_caret,
                             longest_line, end_caret_pos);

    gl->blendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    rect_renderer.draw(size, scroll_offset, end_caret_pos, main_glyph_cache.lineHeight(), 50, 1000,
                       editor_offset, ui_glyph_cache.lineHeight());
}

void Renderer::flush(const Size& size) {
    gl->viewport(0, 0, size.width, size.height);
    gl->clear(GL_COLOR_BUFFER_BIT);
    text_renderer.flush(size);
    rect_renderer.flush(size);
}

}
