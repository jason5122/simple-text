// A GL context with no window system attached at all.
//
// The windowed backend gets its context from GDK, which needs a realized GdkWindow and therefore
// an X or Wayland connection. This path talks to EGL directly and asks for the surfaceless
// platform, so it needs neither: no display server, no GTK, no window. Mesa renders into the
// framebuffer object below and nothing is ever presented.
//
// EGL is loaded from the sysroot at link time; Fedora ships libEGL.so.1, which is the SONAME the
// binary records.

#define EGL_NO_X11
#define MESA_EGL_NO_X11_HEADERS

#include "px/px_offscreen.h"

#include "px/gl_render_context.h"
#include "px/px_gl.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

#ifndef EGL_PLATFORM_SURFACELESS_MESA
#define EGL_PLATFORM_SURFACELESS_MESA 0x31DD
#endif

struct px_offscreen_t {
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLContext context = EGL_NO_CONTEXT;
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

namespace {

// The surfaceless platform is what makes this displayless. Falling back to the default display
// keeps the path working on a machine whose EGL predates the extension, where it will pick up
// whatever window system is configured instead.
EGLDisplay open_display() {
    auto get_platform_display = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
        eglGetProcAddress("eglGetPlatformDisplayEXT"));
    if (get_platform_display) {
        EGLDisplay display =
            get_platform_display(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
        if (display != EGL_NO_DISPLAY) {
            return display;
        }
        std::fprintf(stderr, "px: surfaceless EGL unavailable, trying the default display\n");
    }
    return eglGetDisplay(EGL_DEFAULT_DISPLAY);
}

}  // namespace

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

    surface->display = open_display();
    if (surface->display == EGL_NO_DISPLAY) {
        std::fprintf(stderr, "px: eglGetDisplay failed\n");
        return nullptr;
    }
    EGLint major = 0;
    EGLint minor = 0;
    if (!eglInitialize(surface->display, &major, &minor)) {
        std::fprintf(stderr, "px: eglInitialize failed (0x%x)\n", eglGetError());
        return nullptr;
    }
    // Desktop GL, not GLES: the shared renderer's shaders are 3.3 core.
    if (!eglBindAPI(EGL_OPENGL_API)) {
        std::fprintf(stderr, "px: eglBindAPI(EGL_OPENGL_API) failed (0x%x)\n", eglGetError());
        return nullptr;
    }

    const EGLint config_attributes[] = {EGL_SURFACE_TYPE,
                                        EGL_PBUFFER_BIT,
                                        EGL_RENDERABLE_TYPE,
                                        EGL_OPENGL_BIT,
                                        EGL_RED_SIZE,
                                        8,
                                        EGL_GREEN_SIZE,
                                        8,
                                        EGL_BLUE_SIZE,
                                        8,
                                        EGL_ALPHA_SIZE,
                                        8,
                                        EGL_NONE};
    EGLConfig config = nullptr;
    EGLint config_count = 0;
    if (!eglChooseConfig(surface->display, config_attributes, &config, 1, &config_count) ||
        config_count < 1) {
        std::fprintf(stderr, "px: eglChooseConfig found no config (0x%x)\n", eglGetError());
        return nullptr;
    }

    const EGLint context_attributes[] = {EGL_CONTEXT_MAJOR_VERSION,
                                         3,
                                         EGL_CONTEXT_MINOR_VERSION,
                                         3,
                                         EGL_CONTEXT_OPENGL_PROFILE_MASK,
                                         EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
                                         EGL_NONE};
    surface->context =
        eglCreateContext(surface->display, config, EGL_NO_CONTEXT, context_attributes);
    if (surface->context == EGL_NO_CONTEXT) {
        std::fprintf(stderr, "px: eglCreateContext failed (0x%x)\n", eglGetError());
        return nullptr;
    }
    // EGL_KHR_surfaceless_context: current with no draw or read surface, drawing into an FBO.
    if (!eglMakeCurrent(surface->display, EGL_NO_SURFACE, EGL_NO_SURFACE, surface->context)) {
        std::fprintf(stderr, "px: surfaceless eglMakeCurrent failed (0x%x)\n", eglGetError());
        return nullptr;
    }

    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    std::fprintf(stderr, "px: EGL %d.%d, GL %s, renderer %s\n", major, minor,
                 version ? reinterpret_cast<const char*>(version) : "?",
                 renderer ? reinterpret_cast<const char*>(renderer) : "?");

    glGenFramebuffers(1, &surface->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, surface->fbo);

    glGenRenderbuffers(1, &surface->color);
    glBindRenderbuffer(GL_RENDERBUFFER, surface->color);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, surface->width, surface->height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                              surface->color);

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
    if (surface->display != EGL_NO_DISPLAY) {
        if (surface->color) glDeleteRenderbuffers(1, &surface->color);
        if (surface->stencil) glDeleteRenderbuffers(1, &surface->stencil);
        if (surface->fbo) glDeleteFramebuffers(1, &surface->fbo);
        eglMakeCurrent(surface->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (surface->context != EGL_NO_CONTEXT) {
            eglDestroyContext(surface->display, surface->context);
        }
        eglTerminate(surface->display);
    }
    delete surface;
}

bool px_offscreen_paint(px_offscreen_t* surface, px_window_event_handler* handler) {
    if (!surface || !handler || !surface->fbo) {
        return false;
    }
    if (!eglMakeCurrent(surface->display, EGL_NO_SURFACE, EGL_NO_SURFACE, surface->context)) {
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, surface->fbo);

    handler->pre_paint();

    const vec2 device{static_cast<double>(surface->width), static_cast<double>(surface->height)};
    glViewport(0, 0, static_cast<GLsizei>(surface->width), static_cast<GLsizei>(surface->height));

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
