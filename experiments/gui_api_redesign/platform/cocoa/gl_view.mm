#include "experiments/gui_api_redesign/platform/cocoa/gl_view.h"
#include "gl/gl.h"
#include "gl/loader.h"

using namespace gl;

constexpr bool kBenchmarkMode = false;

@interface GLLayer : CAOpenGLLayer {
    Window* app_window_;
    GLContext* ctx_;
    GLPixelFormat* pf_;
}
- (instancetype)initWithAppWindow:(Window*)appWindow
                        glContext:(GLContext*)ctx
                    glPixelFormat:(GLPixelFormat*)pf;
@end

@implementation GLView {
    Window* app_window_;
}

- (instancetype)initWithFrame:(NSRect)frameRect
                    appWindow:(Window*)appWindow
                    glContext:(GLContext*)ctx
                glPixelFormat:(GLPixelFormat*)pf {
    self = [super initWithFrame:frameRect];
    if (self) {
        app_window_ = appWindow;
        self.wantsLayer = YES;
        self.layer = [[[GLLayer alloc] initWithAppWindow:appWindow glContext:ctx
                                           glPixelFormat:pf] autorelease];
        self.layer.needsDisplayOnBoundsChange = YES;
    }
    return self;
}

@end

@implementation GLLayer

- (instancetype)initWithAppWindow:(Window*)appWindow
                        glContext:(GLContext*)ctx
                    glPixelFormat:(GLPixelFormat*)pf {
    self = [super init];
    if (self) {
        app_window_ = appWindow;
        ctx_ = ctx;
        pf_ = pf;
    }
    return self;
}

- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat {
    CGLSetCurrentContext(ctx_->get());
    gl::load_global_function_pointers();
    return ctx_->get();
}

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
    return pf_->get();
}

- (void)drawInCGLContext:(CGLContextObj)glContext
             pixelFormat:(CGLPixelFormatObj)pixelFormat
            forLayerTime:(CFTimeInterval)timeInterval
             displayTime:(const CVTimeStamp*)timeStamp {
    if constexpr (kBenchmarkMode) {
        [NSApp terminate:nil];
    }

    if (app_window_) {
        app_window_->invoke_draw_callback();
        glFlush();
    }
}

// We shouldn't release the CGLContextObj since it isn't owned by this object.
- (void)releaseCGLContext:(CGLContextObj)glContext {
}

// We shouldn't release the CGLPixelFormatObj since it isn't owned by this object.
- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pixelFormat {
}

@end
