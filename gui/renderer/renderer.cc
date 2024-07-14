#include "build/buildflag.h"
#include "renderer.h"

#include "opengl/gl.h"
using namespace opengl;

// TODO: Properly load this from settings.
namespace {
#if IS_MAC
constexpr int kMainFontSize = 16 * 2;
constexpr std::string kMainFontFace = "SF Pro Text";
#elif IS_WIN || IS_LINUX
constexpr int kMainFontSize = 12 * 2;
constexpr std::string kMainFontFace = "Arial";
#endif

constexpr int kUIFontSize = 11 * 2;
}

namespace gui {

Renderer::Renderer()
    : main_glyph_cache{"Source Code Pro", kMainFontSize},
      ui_glyph_cache{kMainFontFace, kUIFontSize},
      text_renderer{main_glyph_cache, ui_glyph_cache},
      selection_renderer{main_glyph_cache} {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glClearColor(253.0f / 255, 253.0f / 255, 253.0f / 255, 1.0f);
}

Renderer& Renderer::instance() {
    static Renderer renderer;
    return renderer;
}

GlyphCache& Renderer::getMainGlyphCache() {
    return main_glyph_cache;
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

    selection_renderer.render(size, 0);
    text_renderer.flush(size, true);
    selection_renderer.render(size, 1);
    rect_renderer.flush(size);
    image_renderer.flush(size);
    text_renderer.flush(size, false);

    selection_renderer.destroyInstances();
}

}
