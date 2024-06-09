#include "GLLayer.h"

@implementation GLLayer

- (instancetype)initWithDisplayGL:(gui::DisplayGL*)theDisplaygl {
    self = [super init];
    if (self) {
        displaygl = theDisplaygl;
    }
    return self;
}

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
    return displaygl->pixelFormat();
}

- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat {
    CGLSetCurrentContext(displaygl->context());

    int scaled_width = self.frame.size.width * self.contentsScale;
    int scaled_height = self.frame.size.height * self.contentsScale;

    appWindow->onOpenGLActivate(scaled_width, scaled_height);

    [self addObserver:self forKeyPath:@"bounds" options:0 context:nil];

    return displaygl->context();
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
    CGLSetCurrentContext(displaygl->context());

    // TODO: For debugging; remove this.
    // [NSApp terminate:nil];

    appWindow->onDraw();

    // TODO: For debugging; remove this.
    // appWindow->stopLaunchTimer();

    // Calls glFlush() by default.
    [super drawInCGLContext:displaygl->context()
                pixelFormat:pixelFormat
               forLayerTime:timeInterval
                displayTime:timeStamp];
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context {
    CGLSetCurrentContext(displaygl->context());

    float scaled_width = self.frame.size.width * self.contentsScale;
    float scaled_height = self.frame.size.height * self.contentsScale;
    appWindow->onResize(scaled_width, scaled_height);
}

// We shouldn't release the CGLContextObj since it isn't owned by this object.
- (void)releaseCGLContext:(CGLContextObj)glContext {
}

// We shouldn't release the CGLPixelFormatObj since it isn't owned by this object.
- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pixelFormat {
}

@end
