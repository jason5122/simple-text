#include "gl/loader.h"
#include "gl_view.h"
#include <fmt/base.h>

constexpr bool kBenchmarkMode = false;

@interface GLLayer : CAOpenGLLayer {
    Window* app_window_;
    GLContextManager* mgr_;
    CGLContextObj ctx_;
}
- (instancetype)initWithAppWindow:(Window*)appWindow glContextManager:(GLContextManager*)mgr;
@end

@implementation GLView {
    Window* app_window_;
}

- (instancetype)initWithFrame:(NSRect)frameRect
                    appWindow:(Window*)appWindow
             glContextManager:(GLContextManager*)mgr {
    self = [super initWithFrame:frameRect];
    if (self) {
        app_window_ = appWindow;
        self.wantsLayer = YES;
        self.layer = [[[GLLayer alloc] initWithAppWindow:appWindow
                                        glContextManager:mgr] autorelease];
        self.layer.needsDisplayOnBoundsChange = YES;
    }
    return self;
}

@end

@implementation GLLayer

- (instancetype)initWithAppWindow:(Window*)appWindow glContextManager:(GLContextManager*)mgr {
    self = [super init];
    if (self) {
        app_window_ = appWindow;
        mgr_ = mgr;
        ctx_ = mgr->create_layer_context();
    }
    return self;
}

- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat {
    CGLSetCurrentContext(ctx_);
    gl::load_global_function_pointers();

    return ctx_;
}

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
    return mgr_->pixel_format();
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
    }
}

@end
