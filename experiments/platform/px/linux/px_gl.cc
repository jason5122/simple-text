// GDK-native GL context creation, and the FBO/renderbuffer this backend blits from.
//
// Two decisions read straight off ST's binary:
//
//   * Context creation goes through gdk_window_create_gl_context, not raw GLX. The import table
//     has no glX* symbol at all -- ST relies on GDK's wrapper exclusively, the same way the macOS
//     backend relies on CGL instead of hand-rolled pixel-format negotiation.
//
//   * There is no swap-buffers call anywhere (no glXSwapBuffers, no such string), and ST finishes
//     with glFlush -- exactly the Windows finding (glFlush, no SwapBuffers) and the reason
//     kCGLPFABackingStore matters on macOS. All three platforms render into a persistent,
//     compositor-owned drawable rather than swapping one. On GTK3 specifically, "compositor-owned"
//     means cairo: ST allocates its own framebuffer with directly-linked
//     glGenFramebuffers/glFramebufferRenderbuffer/glGenRenderbuffers/glRenderbufferStorage (all
//     confirmed direct imports, unlike the dlsym'd GTK/GDK surface) and hands the result to
//     gdk_cairo_draw_from_gl, which composites it into the window the normal cairo way. That FBO
//     is reproduced here.
//
// GDK shares GL object namespaces implicitly across every context created against the same
// GdkDisplay, so unlike the Windows backend's explicit wglShareLists dance, one context per window
// is already enough to get ST's "one shared namespace" property for free.

#include "experiments/platform/px/linux/px_linux_private.h"

#include <cstdio>

bool px_linux_gl_create(px_window_t* window) {
    if (!window || !window->area) {
        return false;
    }
    GdkWindow* gdk_win = gtk_widget_get_window(window->area);
    if (!gdk_win) {
        return false;
    }

    GError* error = nullptr;
    window->gl_context = gdk_window_create_gl_context(gdk_win, &error);
    if (!window->gl_context) {
        std::fprintf(stderr, "px: gdk_window_create_gl_context failed: %s\n",
                     error ? error->message : "?");
        if (error) g_error_free(error);
        return false;
    }

    gdk_gl_context_set_required_version(window->gl_context, 4, 1);
    if (!gdk_gl_context_realize(window->gl_context, &error)) {
        std::fprintf(stderr, "px: gdk_gl_context_realize failed: %s\n",
                     error ? error->message : "?");
        if (error) g_error_free(error);
        g_object_unref(window->gl_context);
        window->gl_context = nullptr;
        return false;
    }

    gdk_gl_context_make_current(window->gl_context);
    const GLubyte* version = glGetString(GL_VERSION);
    std::fprintf(stderr, "px: GL %s, legacy=%d\n",
                 version ? reinterpret_cast<const char*>(version) : "?",
                 gdk_gl_context_is_legacy(window->gl_context) ? 1 : 0);
    return true;
}

bool px_linux_gl_make_current(px_window_t* window) {
    if (!window || !window->gl_context) {
        return false;
    }
    gdk_gl_context_make_current(window->gl_context);
    return true;
}

bool px_linux_gl_ensure_target(px_window_t* window, int width, int height) {
    if (!window || !window->gl_context || width < 1 || height < 1) {
        return false;
    }
    if (window->fbo != 0 && window->fbo_width == width && window->fbo_height == height) {
        return true;
    }

    if (window->fbo != 0) {
        glDeleteFramebuffers(1, &window->fbo);
        glDeleteRenderbuffers(1, &window->color_renderbuffer);
        glDeleteRenderbuffers(1, &window->stencil_renderbuffer);
        window->fbo = 0;
        window->color_renderbuffer = 0;
        window->stencil_renderbuffer = 0;
    }

    glGenRenderbuffers(1, &window->color_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, window->color_renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);

    glGenFramebuffers(1, &window->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, window->fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                              window->color_renderbuffer);

    glGenRenderbuffers(1, &window->stencil_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, window->stencil_renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              window->stencil_renderbuffer);

    const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (!complete) {
        std::fprintf(stderr, "px: GL framebuffer incomplete\n");
        glDeleteFramebuffers(1, &window->fbo);
        glDeleteRenderbuffers(1, &window->color_renderbuffer);
        glDeleteRenderbuffers(1, &window->stencil_renderbuffer);
        window->fbo = 0;
        window->color_renderbuffer = 0;
        window->stencil_renderbuffer = 0;
        return false;
    }

    window->fbo_width = width;
    window->fbo_height = height;
    // The replacement color attachment has undefined contents. Make the next draw reconstruct it
    // completely instead of trusting the widget's potentially smaller expose clip.
    window->did_first_paint = false;
    return true;
}

void px_linux_gl_destroy(px_window_t* window) {
    if (!window) {
        return;
    }
    if (window->gl_context) {
        gdk_gl_context_make_current(window->gl_context);
        if (window->fbo != 0) {
            glDeleteFramebuffers(1, &window->fbo);
            glDeleteRenderbuffers(1, &window->color_renderbuffer);
            glDeleteRenderbuffers(1, &window->stencil_renderbuffer);
            window->fbo = 0;
            window->color_renderbuffer = 0;
            window->stencil_renderbuffer = 0;
        }
        gdk_gl_context_clear_current();
        g_object_unref(window->gl_context);
        window->gl_context = nullptr;
    }
}
