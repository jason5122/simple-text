#include "gl_layer.h"

// TODO: Debug use; remove this.
#include <Cocoa/Cocoa.h>
#include <fmt/base.h>

@interface GLLayer () {
    app::Window* app_window;
    app::DisplayGL* display_gl;
}

@end

@implementation GLLayer

- (instancetype)initWithAppWindow:(app::Window*)appWindow displayGL:(app::DisplayGL*)displayGL {
    self = [super init];
    if (self) {
        app_window = appWindow;
        display_gl = displayGL;
    }
    return self;
}

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
    return display_gl->pixelFormat();
}

- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat {
    // Set up KVO to detect resizing.
    [self addObserver:self forKeyPath:@"bounds" options:0 context:nil];

    // Call OpenGL activation callback.
    CGLSetCurrentContext(display_gl->context());
    int scaled_width = self.frame.size.width * self.contentsScale;
    int scaled_height = self.frame.size.height * self.contentsScale;
    app_window->onOpenGLActivate({scaled_width, scaled_height});
    return display_gl->context();
}

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
    // TODO: For debugging; remove this.
    // [NSApp terminate:nil];

    CGLSetCurrentContext(glContext);

    int scaled_width = self.frame.size.width * self.contentsScale;
    int scaled_height = self.frame.size.height * self.contentsScale;
    app_window->onDraw({scaled_width, scaled_height});

    // Calls glFlush() by default.
    [super drawInCGLContext:glContext
                pixelFormat:pixelFormat
               forLayerTime:timeInterval
                displayTime:timeStamp];

    // TODO: For debugging; remove this.
    // [NSApp terminate:nil];
}

// TODO: GLLayer calling `onResize()` is currently known to redraw after the window has closed if
// NSWindowTabbingMode is enabled. Fix this.
- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context {
    CGLSetCurrentContext(display_gl->context());

    int scaled_width = self.frame.size.width * self.contentsScale;
    int scaled_height = self.frame.size.height * self.contentsScale;
    app_window->onResize({scaled_width, scaled_height});
}

// We shouldn't release the CGLContextObj since it isn't owned by this object.
- (void)releaseCGLContext:(CGLContextObj)glContext {
}

// We shouldn't release the CGLPixelFormatObj since it isn't owned by this object.
- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pixelFormat {
}

@end
