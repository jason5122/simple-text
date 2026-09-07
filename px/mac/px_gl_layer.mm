// The CAOpenGLLayer backing.
//
// ST's is `OpenGLLayer : CAOpenGLLayer`, installed from -[PXView makeBackingLayer]. Everything
// below that looks unusual is copied from what the binary does:
//
//   * copyCGLPixelFormatForDisplayMask: and copyCGLContextForPixelFormat: return process-wide
//     globals, and the matching release methods are literally `ret`. Every window shares one GL
//     context, so glyph atlases and buffers are shared for free.
//
//   * The pixel format request, read out of __TEXT,__const at 0x100509e44, is
//     {kCGLPFAColorSize 24, kCGLPFAAlphaSize 8, kCGLPFAOpenGLProfile 0x4100 (4.1 Core),
//      kCGLPFANoRecovery, kCGLPFAAllowOfflineRenderers, kCGLPFABackingStore, 0}.
//
//   * setAsynchronous:NO. Drawing is driven by setNeedsDisplayInRect: and the display link, not by
//     Core Animation polling canDrawInCGLContext:.
//
//   * Rendering goes into a layer-owned persistent FBO. Its RGBA8 color and 8-bit stencil
//     renderbuffers grow with 30 pixels of slack, matching the binary. Dirty regions update that
//     stable image, then the complete visible area is blitted into CA's current drawable.
//
// Known limitation of CAOpenGLLayer itself, reproduced with a bare layer and no px: when the
// window grows quickly (tens of points per frame) the Core Animation commit occasionally blocks
// the main thread for ~500 ms before drawInCGLContext: is called. CAMetalLayer does not, which is
// why Metal is the default backing and this layer is opt-in with PX_GL=1.

#include "px/gl_render_context.h"
#include "px/mac/px_mac_private.h"
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl3.h>
#include <algorithm>
#include <mutex>
#include <vector>

// CAOpenGLLayer and CGL are deprecated as of macOS 10.14; ST 4200 still ships on them, and
// mirroring that is the point of this experiment.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace {

CGLPixelFormatObj g_pixel_format = nullptr;
CGLContextObj g_context = nullptr;

// Created once, on first use, and never released. ST does the same: the globals live at
// 0x100623578 / 0x100623580 and the layer's release callbacks are no-ops.
void ensure_shared_gl() {
    if (g_context) {
        return;
    }

    const CGLPixelFormatAttribute attributes[] = {
        kCGLPFAColorSize,     static_cast<CGLPixelFormatAttribute>(24),
        kCGLPFAAlphaSize,     static_cast<CGLPixelFormatAttribute>(8),
        kCGLPFAOpenGLProfile, static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_GL4_Core),
        kCGLPFANoRecovery,    kCGLPFAAllowOfflineRenderers,
        kCGLPFABackingStore,  static_cast<CGLPixelFormatAttribute>(0),
    };

    GLint npix = 0;
    if (CGLChoosePixelFormat(attributes, &g_pixel_format, &npix) != kCGLNoError ||
        !g_pixel_format) {
        NSLog(@"px: CGLChoosePixelFormat failed");
        return;
    }
    if (CGLCreateContext(g_pixel_format, nullptr, &g_context) != kCGLNoError) {
        NSLog(@"px: CGLCreateContext failed");
        g_context = nullptr;
        return;
    }

    // ST writes zero to kCGLCPSwapInterval immediately after creating the process-wide context
    // (CGLSetParameter parameter 0xde). The value turns out not to matter either way: setting it to
    // 1 was measured to leave the redraw rate unchanged, because presentation goes through
    // -[CAOpenGLLayer drawInCGLContext:] rather than a CGL drawable swap. Nothing reachable from
    // CGL paces this renderer against the display; only a display link can.
    const GLint swap_interval = 0;
    CGLSetParameter(g_context, kCGLCPSwapInterval, &swap_interval);
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────────────────────────

@interface PXOpenGLLayer : CAOpenGLLayer {
    px_window_t* _pxw;
    std::vector<rect> _dirty;
    std::mutex _dirtyMutex;
    GLuint _framebuffer;
    GLuint _colorRenderbuffer;
    GLuint _stencilRenderbuffer;
    GLsizei _backingWidth;
    GLsizei _backingHeight;
    GLsync _presentationFence;
}
- (instancetype)initWithPXW:(px_window_t*)pxw;
- (void)addDirtyRect:(rect)r;
@end

@implementation PXOpenGLLayer

- (instancetype)initWithPXW:(px_window_t*)pxw {
    self = [super init];
    if (self) {
        _pxw = pxw;
        _framebuffer = 0;
        _colorRenderbuffer = 0;
        _stencilRenderbuffer = 0;
        _backingWidth = 0;
        _backingHeight = 0;
        _presentationFence = nullptr;
        // -[OpenGLLayer initWithPXW:] seeds the list with a full-size rect so the first frame is a
        // complete repaint.
        _dirty.push_back(rect{0.0, 0.0, px_window_size(pxw).x, px_window_size(pxw).y});
    }
    return self;
}

- (void)addDirtyRect:(rect)r {
    const std::lock_guard lock(_dirtyMutex);
    _dirty.push_back(r);
}

- (void)dealloc {
    if (g_context &&
        (_framebuffer || _colorRenderbuffer || _stencilRenderbuffer || _presentationFence)) {
        CGLContextObj previous = CGLGetCurrentContext();
        CGLLockContext(g_context);
        CGLSetCurrentContext(g_context);
        if (_presentationFence) glDeleteSync(_presentationFence);
        if (_stencilRenderbuffer) glDeleteRenderbuffers(1, &_stencilRenderbuffer);
        if (_colorRenderbuffer) glDeleteRenderbuffers(1, &_colorRenderbuffer);
        if (_framebuffer) glDeleteFramebuffers(1, &_framebuffer);
        CGLSetCurrentContext(previous);
        CGLUnlockContext(g_context);
    }
}

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
    ensure_shared_gl();
    return g_pixel_format;
}

- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pf {
    // Intentionally empty: the pixel format is a shared global. `ret`, same as ST.
}

- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pf {
    ensure_shared_gl();
    return g_context;
}

- (void)releaseCGLContext:(CGLContextObj)ctx {
    // Intentionally empty: the context is a shared global.
}

- (void)drawInCGLContext:(CGLContextObj)ctx
             pixelFormat:(CGLPixelFormatObj)pf
            forLayerTime:(CFTimeInterval)t
             displayTime:(const CVTimeStamp*)ts {
    if (!_pxw || !_pxw->handler) {
        return;
    }

    const double scale = px_window_dpi_scale_factor(_pxw);
    self.contentsScale = scale;

    const vec2 size = px_window_size(_pxw);
    const vec2 device{size.x * scale, size.y * scale};
    const GLsizei width = static_cast<GLsizei>(device.x);
    const GLsizei height = static_cast<GLsizei>(device.y);
    if (width < 1 || height < 1) {
        return;
    }

    // One submitted frame outstanding. ST gets equivalent back-pressure from reusing its small
    // ring of dynamic GL buffers; a fence models that limit for scenes too light to hit it.
    if (_presentationFence) {
        for (;;) {
            const GLenum result =
                glClientWaitSync(_presentationFence, GL_SYNC_FLUSH_COMMANDS_BIT, 1'000'000'000ULL);
            if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED ||
                result == GL_WAIT_FAILED) {
                break;
            }
        }
        glDeleteSync(_presentationFence);
        _presentationFence = nullptr;
    }

    std::vector<rect> dirty;
    {
        const std::lock_guard lock(_dirtyMutex);
        dirty.swap(_dirty);
    }
    // AppKit displays the layer synchronously inside a window resize, after windowDidResize: has
    // marked the window dirty but before the run-loop observer has handed those rectangles to the
    // layer. Take them here: this frame draws them, and the later flush then finds nothing and
    // schedules no second display of content already on screen.
    dirty.insert(dirty.end(), _pxw->dirty.begin(), _pxw->dirty.end());
    _pxw->dirty.clear();
    if (dirty.empty()) {
        dirty.push_back(rect{0.0, 0.0, size.x, size.y});
    }

    _pxw->handler->pre_paint();
    px_mac_dispatch_post_event_callbacks();

    GLint destinationDrawFramebuffer = 0;
    GLint previousReadFramebuffer = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &destinationDrawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);

    if (!_framebuffer) {
        glGenFramebuffers(1, &_framebuffer);
        glGenRenderbuffers(1, &_colorRenderbuffer);
        glGenRenderbuffers(1, &_stencilRenderbuffer);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    if (_backingWidth < width || _backingHeight < height) {
        _backingWidth = std::max(_backingWidth, width + 30);
        _backingHeight = std::max(_backingHeight, height + 30);

        glBindRenderbuffer(GL_RENDERBUFFER, _colorRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, _backingWidth, _backingHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                                  _colorRenderbuffer);

        glBindRenderbuffer(GL_RENDERBUFFER, _stencilRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, _backingWidth,
                              _backingHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                  _stencilRenderbuffer);

        // Storage was replaced, so every visible pixel must be reconstructed this frame.
        dirty.clear();
        dirty.push_back(rect{0.0, 0.0, size.x, size.y});
    }

    const bool framebufferComplete =
        glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (!framebufferComplete) {
        NSLog(@"px: persistent framebuffer is incomplete; drawing directly to CA drawable");
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(destinationDrawFramebuffer));
        // A CA drawable is not our persistent backing store. Reconstruct it completely, and use
        // only the coarse scissor path: the drawable has no stencil.
        dirty.clear();
        dirty.push_back(rect{0.0, 0.0, size.x, size.y});
    }

    glViewport(0, 0, width, height);
    const rect windowBounds{0.0, 0.0, size.x, size.y};
    gl_render_context::normalize_dirty_rects(&dirty, windowBounds);
    {
        gl_render_context rc(device, scale, dirty.data(), static_cast<int>(dirty.size()),
                             framebufferComplete);
        _pxw->handler->paint(&rc, rc.paint_bounds(), dirty.data(),
                             static_cast<int>(dirty.size()));
        rc.finish();
    }

    if (framebufferComplete) {
        glDisable(GL_SCISSOR_TEST);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(destinationDrawFramebuffer));
        glBindFramebuffer(GL_READ_FRAMEBUFFER, _framebuffer);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT,
                          GL_NEAREST);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previousReadFramebuffer));
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(destinationDrawFramebuffer));

    _pxw->did_first_paint = true;

    // Flushes and swaps.
    [super drawInCGLContext:ctx pixelFormat:pf forLayerTime:t displayTime:ts];

    _presentationFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

@end

// ─────────────────────────────────────────────────────────────────────────────────────────────────

CALayer* px_mac_make_gl_layer(px_window_t* window) {
    PXOpenGLLayer* layer = [[PXOpenGLLayer alloc] initWithPXW:window];

    // Not asynchronous: redraws come from setNeedsDisplayInRect: and the display link.
    layer.asynchronous = NO;
    layer.needsDisplayOnBoundsChange = YES;
    layer.opaque = window->background.a >= 1.0f;
    layer.contentsScale = px_window_dpi_scale_factor(window);

    return layer;
}

void px_mac_gl_layer_add_dirty(CALayer* layer, rect r) {
    if ([layer isKindOfClass:[PXOpenGLLayer class]]) {
        [static_cast<PXOpenGLLayer*>(layer) addDirtyRect:r];
    }
}

#pragma clang diagnostic pop
