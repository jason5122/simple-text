#include "renderer_lite.h"

#include "opengl/gl.h"
using namespace opengl;

namespace gui {

static_assert(std::is_move_constructible_v<RendererLite>);
static_assert(std::is_move_assignable_v<RendererLite>);

RendererLite::RendererLite() {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
}

void RendererLite::flush(const Size& size, const Rgb& color) {
    glViewport(0, 0, size.width, size.height);
    glClearColor(static_cast<float>(color.r) / 255, static_cast<float>(color.g) / 255,
                 static_cast<float>(color.b) / 255, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

}  // namespace gui
