// The OpenGL half of macOS's offscreen surface, selected by PX_GL.
//
// CGL needs no drawable: a context can be current with nothing attached, and everything is drawn
// into a framebuffer object. That makes this the one platform where both renderers can capture the
// same scene on the same machine, which is the point of having it -- Metal and GL disagreeing on a
// capture is a renderer bug, not a platform difference.

#include "px/mac/px_offscreen_mac.h"

#include "px/gl_render_context.h"
#include "px/px_gl.h"
#import <OpenGL/OpenGL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

namespace {

class gl_offscreen final : public px_mac_offscreen {
public:
    ~gl_offscreen() override {
        if (!context_) {
            return;
        }
        CGLSetCurrentContext(context_);
        if (color_) glDeleteRenderbuffers(1, &color_);
        if (stencil_) glDeleteRenderbuffers(1, &stencil_);
        if (fbo_) glDeleteFramebuffers(1, &fbo_);
        CGLSetCurrentContext(nullptr);
        CGLDestroyContext(context_);
    }

    bool create(double width, double height, double dpi_scale) {
        bounds_ = {.x = 0.0, .y = 0.0, .w = width, .h = height};
        dpi_scale_ = dpi_scale;
        width_ = static_cast<int>(std::lround(width * dpi_scale));
        height_ = static_cast<int>(std::lround(height * dpi_scale));
        if (width_ <= 0 || height_ <= 0) {
            return false;
        }

        // 3.2 core is the newest profile CGL names; on any Mac that runs this it yields 4.1, which
        // is what the CAOpenGLLayer's context reports too.
        const CGLPixelFormatAttribute attributes[] = {
            kCGLPFAOpenGLProfile,
            static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_3_2_Core),
            kCGLPFAColorSize,
            static_cast<CGLPixelFormatAttribute>(24),
            kCGLPFAAlphaSize,
            static_cast<CGLPixelFormatAttribute>(8),
            kCGLPFAAccelerated,
            static_cast<CGLPixelFormatAttribute>(0),
        };
        CGLPixelFormatObj format = nullptr;
        GLint format_count = 0;
        if (CGLChoosePixelFormat(attributes, &format, &format_count) != kCGLNoError || !format) {
            std::fprintf(stderr, "px: CGLChoosePixelFormat found no offscreen format\n");
            return false;
        }
        const CGLError error = CGLCreateContext(format, nullptr, &context_);
        CGLDestroyPixelFormat(format);
        if (error != kCGLNoError || !context_) {
            std::fprintf(stderr, "px: CGLCreateContext failed (%d)\n", error);
            return false;
        }
        CGLSetCurrentContext(context_);

        const GLubyte* version = glGetString(GL_VERSION);
        const GLubyte* renderer = glGetString(GL_RENDERER);
        std::fprintf(stderr, "px: CGL offscreen, GL %s, renderer %s\n",
                     version ? reinterpret_cast<const char*>(version) : "?",
                     renderer ? reinterpret_cast<const char*>(renderer) : "?");

        glGenFramebuffers(1, &fbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

        glGenRenderbuffers(1, &color_);
        glBindRenderbuffer(GL_RENDERBUFFER, color_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width_, height_);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_);

        glGenRenderbuffers(1, &stencil_);
        glBindRenderbuffer(GL_RENDERBUFFER, stencil_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, width_, height_);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                  stencil_);
        has_stencil_ = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        if (!has_stencil_) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
            glDeleteRenderbuffers(1, &stencil_);
            stencil_ = 0;
        }

        const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::fprintf(stderr, "px: offscreen framebuffer incomplete (0x%x)\n", status);
            return false;
        }

        pixels_.resize(static_cast<size_t>(width_) * static_cast<size_t>(height_));
        scratch_row_.resize(static_cast<size_t>(width_));
        return true;
    }

    bool paint(px_window_event_handler* handler) override {
        if (!context_ || !fbo_) {
            return false;
        }
        CGLSetCurrentContext(context_);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

        handler->pre_paint();

        const vec2 device{static_cast<double>(width_), static_cast<double>(height_)};
        glViewport(0, 0, static_cast<GLsizei>(width_), static_cast<GLsizei>(height_));

        std::vector<rect> dirty{bounds_};
        gl_render_context::normalize_dirty_rects(&dirty, bounds_);
        {
            gl_render_context rc(device, dpi_scale_, dirty.data(), static_cast<int>(dirty.size()),
                                 has_stencil_);
            handler->paint(&rc, rc.paint_bounds(), dirty.data(), static_cast<int>(dirty.size()));
            rc.finish();
        }
        glFinish();

        // glReadPixels hands back rows bottom-up; px_offscreen_pixels is documented top-down.
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, static_cast<GLsizei>(width_), static_cast<GLsizei>(height_), GL_BGRA,
                     GL_UNSIGNED_INT_8_8_8_8_REV, pixels_.data());

        const size_t row = static_cast<size_t>(width_);
        for (int y = 0; y < height_ / 2; ++y) {
            uint32_t* top = pixels_.data() + static_cast<size_t>(y) * row;
            uint32_t* bottom = pixels_.data() + static_cast<size_t>(height_ - 1 - y) * row;
            std::copy_n(top, row, scratch_row_.data());
            std::copy_n(bottom, row, top);
            std::copy_n(scratch_row_.data(), row, bottom);
        }

        painted_ = true;
        return true;
    }

    const uint32_t* pixels() const override { return painted_ ? pixels_.data() : nullptr; }
    int width() const override { return width_; }
    int height() const override { return height_; }

private:
    CGLContextObj context_ = nullptr;
    GLuint fbo_ = 0;
    GLuint color_ = 0;
    GLuint stencil_ = 0;
    bool has_stencil_ = false;
    rect bounds_;
    double dpi_scale_ = 1.0;
    int width_ = 0;
    int height_ = 0;
    std::vector<uint32_t> pixels_;
    std::vector<uint32_t> scratch_row_;
    bool painted_ = false;
};

}  // namespace

std::unique_ptr<px_mac_offscreen> px_create_gl_offscreen(double width,
                                                         double height,
                                                         double dpi_scale) {
    if (!(width > 0.0) || !(height > 0.0) || !(dpi_scale > 0.0)) {
        return nullptr;
    }
    auto surface = std::make_unique<gl_offscreen>();
    return surface->create(width, height, dpi_scale) ? std::move(surface) : nullptr;
}
