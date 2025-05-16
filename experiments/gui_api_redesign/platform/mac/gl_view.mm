#include "gl_view.h"
#include <fmt/base.h>

constexpr bool kBenchmarkMode = false;

@interface GLLayer : CAOpenGLLayer {
    Window* app_window_;
}
@property(nonatomic) Window* appWindow;
@end

@implementation GLView {
    Window* app_window_;
}

- (instancetype)initWithFrame:(NSRect)frameRect appWindow:(Window*)appWindow {
    self = [super initWithFrame:frameRect];
    if (self) {
        app_window_ = appWindow;
    }
    return self;
}

- (BOOL)wantsLayer {
    return YES;
}

- (CALayer*)makeBackingLayer {
    auto layer = [GLLayer layer];
    layer.needsDisplayOnBoundsChange = YES;
    layer.appWindow = app_window_;
    return layer;
}

@end

@implementation GLLayer

@synthesize appWindow;

- (BOOL)canDrawInCGLContext:(CGLContextObj)glContext
                pixelFormat:(CGLPixelFormatObj)pixelFormat
               forLayerTime:(CFTimeInterval)timeInterval
                displayTime:(const CVTimeStamp*)timeStamp {
    return true;
}

- (void)drawInCGLContext:(CGLContextObj)glContext
             pixelFormat:(CGLPixelFormatObj)pixelFormat
            forLayerTime:(CFTimeInterval)timeInterval
             displayTime:(const CVTimeStamp*)timeStamp {
    if constexpr (kBenchmarkMode) {
        [NSApp terminate:nil];
    }

    if (self.appWindow) {
        self.appWindow->invoke_draw_callback();
    }
}
@end
