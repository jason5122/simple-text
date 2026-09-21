#include "px/px_offscreen.h"

#include "px/gl_render_context.h"
#include "px/px_gl.h"
#include "px/win/px_win_private.h"
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

// A hidden window carries the GL context. WGL has no context without an HDC, and px_create_window
// already does the pixel-format negotiation, the 3.3 version check and the entry-point loading
// that a context needs -- so the window exists only to own those, and is never shown, never
// pumped and never painted through WM_PAINT. Drawing goes into the framebuffer object below.

namespace {

// px_create_window stores a handler; nothing dispatches to it here because no message loop runs.
class idle_handler final : public px_window_event_handler {
public:
    bool handle_event(px_event_t*) override { return false; }
    void paint(px_render_context*, rect, const rect*, int) override {}
};

}  // namespace

struct px_offscreen_t {
    idle_handler idle;
    px_window_t* window = nullptr;
    GLuint fbo = 0;
    GLuint color = 0;
    GLuint stencil = 0;
    bool has_stencil = false;
    rect bounds;
    double dpi_scale = 1.0;
    int width = 0;
    int height = 0;
    std::vector<uint32_t> pixels;
    std::vector<uint32_t> scratch_row;
    bool painted = false;
};

px_offscreen_t* px_create_offscreen(double width, double height, double dpi_scale) {
    if (!(width > 0.0) || !(height > 0.0) || !(dpi_scale > 0.0)) {
        return nullptr;
    }

    auto surface = std::make_unique<px_offscreen_t>();
    surface->bounds = {.x = 0.0, .y = 0.0, .w = width, .h = height};
    surface->dpi_scale = dpi_scale;
    surface->width = static_cast<int>(std::lround(width * dpi_scale));
    surface->height = static_cast<int>(std::lround(height * dpi_scale));
    if (surface->width <= 0 || surface->height <= 0) {
        return nullptr;
    }

    // One pixel: nothing is ever drawn into this window's default framebuffer, so its size only
    // decides how much backing store the driver allocates for a surface we never touch.
    surface->window = px_create_window(&surface->idle, nullptr, 1.0, 1.0, "px offscreen",
                                       color::from_normalised(0.0f, 0.0f, 0.0f, 1.0f), 0);
    if (!surface->window) {
        std::fprintf(stderr, "px: offscreen window creation failed\n");
        return nullptr;
    }
    if (!px_gl_has_shaders()) {
        std::fprintf(stderr, "px: offscreen needs the GL path; this driver has no shaders\n");
        px_destroy_window(surface->window);
        return nullptr;
    }
    px_win_gl_make_current(surface->window);

    glGenFramebuffers(1, &surface->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, surface->fbo);

    glGenRenderbuffers(1, &surface->color);
    glBindRenderbuffer(GL_RENDERBUFFER, surface->color);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, surface->width, surface->height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                              surface->color);

    // The dirty-rectangle mask wants a stencil, exactly as the windowed path does. Losing it only
    // costs the exact mask, so a driver without one still captures.
    glGenRenderbuffers(1, &surface->stencil);
    glBindRenderbuffer(GL_RENDERBUFFER, surface->stencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, surface->width, surface->height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              surface->stencil);
    surface->has_stencil = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (!surface->has_stencil) {
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
        glDeleteRenderbuffers(1, &surface->stencil);
        surface->stencil = 0;
    }

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::fprintf(stderr, "px: offscreen framebuffer incomplete (0x%x)\n", status);
        px_destroy_window(surface->window);
        return nullptr;
    }

    const size_t count = static_cast<size_t>(surface->width) * static_cast<size_t>(surface->height);
    surface->pixels.resize(count);
    surface->scratch_row.resize(static_cast<size_t>(surface->width));
    return surface.release();
}

void px_destroy_offscreen(px_offscreen_t* surface) {
    if (!surface) {
        return;
    }
    if (surface->window) {
        px_win_gl_make_current(surface->window);
        if (surface->color) glDeleteRenderbuffers(1, &surface->color);
        if (surface->stencil) glDeleteRenderbuffers(1, &surface->stencil);
        if (surface->fbo) glDeleteFramebuffers(1, &surface->fbo);
        px_destroy_window(surface->window);
    }
    delete surface;
}

bool px_offscreen_paint(px_offscreen_t* surface, px_window_event_handler* handler) {
    if (!surface || !handler || !surface->window || !surface->fbo) {
        return false;
    }
    px_win_gl_make_current(surface->window);
    glBindFramebuffer(GL_FRAMEBUFFER, surface->fbo);

    handler->pre_paint();

    const vec2 device{static_cast<double>(surface->width), static_cast<double>(surface->height)};
    glViewport(0, 0, static_cast<GLsizei>(surface->width),
               static_cast<GLsizei>(surface->height));

    std::vector<rect> dirty{surface->bounds};
    gl_render_context::normalize_dirty_rects(&dirty, surface->bounds);
    {
        gl_render_context rc(device, surface->dpi_scale, dirty.data(),
                             static_cast<int>(dirty.size()), surface->has_stencil);
        handler->paint(&rc, rc.paint_bounds(), dirty.data(), static_cast<int>(dirty.size()));
        rc.finish();
    }
    glFinish();

    // glReadPixels hands back rows bottom-up; px_offscreen_pixels is documented top-down, so the
    // rows are reversed in place afterwards.
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, static_cast<GLsizei>(surface->width),
                 static_cast<GLsizei>(surface->height), GL_BGRA, GL_UNSIGNED_BYTE,
                 surface->pixels.data());

    const size_t row = static_cast<size_t>(surface->width);
    for (int y = 0; y < surface->height / 2; ++y) {
        uint32_t* top = surface->pixels.data() + static_cast<size_t>(y) * row;
        uint32_t* bottom =
            surface->pixels.data() + static_cast<size_t>(surface->height - 1 - y) * row;
        std::copy_n(top, row, surface->scratch_row.data());
        std::copy_n(bottom, row, top);
        std::copy_n(surface->scratch_row.data(), row, bottom);
    }

    surface->painted = true;
    return true;
}

const uint32_t* px_offscreen_pixels(const px_offscreen_t* surface) {
    return surface && surface->painted ? surface->pixels.data() : nullptr;
}

int px_offscreen_width(const px_offscreen_t* surface) { return surface ? surface->width : 0; }

int px_offscreen_height(const px_offscreen_t* surface) { return surface ? surface->height : 0; }
