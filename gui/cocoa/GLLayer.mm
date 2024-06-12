#include "GLLayer.h"

// TODO: For debugging; remove this.
#include <Cocoa/Cocoa.h>
#include <iostream>

@interface GLLayer () {
    gui::DisplayGL* mDisplayGL;
}

@end

@implementation GLLayer

- (instancetype)initWithDisplayGL:(gui::DisplayGL*)displayGL {
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
    appWindow->onOpenGLActivate(scaled_width, scaled_height);

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
    CGLSetCurrentContext(glContext);

    // TODO: For debugging; remove this.
    // [NSApp terminate:nil];

    CGLContextObj currentContext = CGLGetCurrentContext();
    std::cerr << currentContext << '\n';

    appWindow->onDraw();

    // TODO: For debugging; remove this.
    // appWindow->stopLaunchTimer();

    // Calls glFlush() by default.
    [super drawInCGLContext:glContext
                pixelFormat:pixelFormat
               forLayerTime:timeInterval
                displayTime:timeStamp];
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context {
    CGLSetCurrentContext(mDisplayGL->context());

    float scaled_width = self.frame.size.width * self.contentsScale;
    float scaled_height = self.frame.size.height * self.contentsScale;
    appWindow->onResize(scaled_width, scaled_height);
}

@end
