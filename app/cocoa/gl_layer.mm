#include "gl_layer.h"

// TODO: Debug use; remove this.
#include <Cocoa/Cocoa.h>

@interface GLLayer () {
    app::DisplayGL* mDisplayGL;
}

@end

@implementation GLLayer

- (instancetype)initWithDisplayGL:(app::DisplayGL*)displayGL {
    self = [super init];
    if (self) {
        mDisplayGL = displayGL;
    }
    return self;
}

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
    return mDisplayGL->pixelFormat();
}

- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat {
    // Set up KVO to detect resizing.
    [self addObserver:self forKeyPath:@"bounds" options:0 context:nil];

    // Call OpenGL activation callback.
    CGLSetCurrentContext(mDisplayGL->context());
    int scaled_width = self.frame.size.width * self.contentsScale;
    int scaled_height = self.frame.size.height * self.contentsScale;
    appWindow->onOpenGLActivate({scaled_width, scaled_height});

    return mDisplayGL->context();
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
    appWindow->onDraw({scaled_width, scaled_height});

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
    CGLSetCurrentContext(mDisplayGL->context());

    int scaled_width = self.frame.size.width * self.contentsScale;
    int scaled_height = self.frame.size.height * self.contentsScale;
    appWindow->onResize({scaled_width, scaled_height});
}

// We shouldn't release the CGLContextObj since it isn't owned by this object.
- (void)releaseCGLContext:(CGLContextObj)glContext {
}

// We shouldn't release the CGLPixelFormatObj since it isn't owned by this object.
- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pixelFormat {
}

@end
