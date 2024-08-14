#include "build/build_config.h"
#include "renderer.h"

#include "opengl/gl.h"
using namespace opengl;

// TODO: Properly load this from settings.
namespace {

#if BUILDFLAG(IS_MAC)
constexpr int kMainFontSize = 16 * 2;
constexpr int kUIFontSize = 11 * 2;
constexpr std::string kMainFontFace = "Menlo";
constexpr std::string kUIFontFace = "SF Pro Text";
#elif BUILDFLAG(IS_WIN)
constexpr int kMainFontSize = 11 * 2;
constexpr int kUIFontSize = 8 * 2;
constexpr std::string kMainFontFace = "Source Code Pro";
// constexpr std::string kMainFontFace = "Consolas";
// constexpr std::string kMainFontFace = "Cascadia Code";
constexpr std::string kUIFontFace = "Segoe UI";
#elif BUILDFLAG(IS_LINUX)
constexpr int kMainFontSize = 12 * 2;
constexpr int kUIFontSize = 11 * 2;
constexpr std::string kMainFontFace = "Monospace";
constexpr std::string kUIFontFace = "Arial";
#endif

// constexpr std::string kMainFontFace = "Source Code Pro";
// constexpr std::string kMainFontFace = "Fira Code";

}

namespace gui {

Renderer::Renderer()
    : glyph_cache{kMainFontFace, kMainFontSize, kUIFontFace, kUIFontSize},
      text_renderer{glyph_cache},
      selection_renderer{glyph_cache} {
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
    text_renderer.flush(size, true);
    rect_renderer.flush(size);
    image_renderer.flush(size);
    text_renderer.flush(size, false);
}

}
