#include "GLLayer.h"

// TODO: For debugging; remove this.
#include <Cocoa/Cocoa.h>

@interface GLLayer () {
    CGLContextObj mDisplayContext;
}

@end

@implementation GLLayer

- (instancetype)initWithContext:(CGLContextObj)displayContext {
    self = [super init];
    if (self) {
        mDisplayContext = displayContext;
    }
    return self;
}

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
    CGLPixelFormatAttribute attribs[] = {
        kCGLPFADisplayMask, static_cast<CGLPixelFormatAttribute>(mask), kCGLPFAOpenGLProfile,
        static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_3_2_Core),
        static_cast<CGLPixelFormatAttribute>(0)};

    CGLPixelFormatObj pixelFormat = nullptr;
    GLint numFormats = 0;
    CGLChoosePixelFormat(attribs, &pixelFormat, &numFormats);

    return pixelFormat;
}

- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat {
    CGLContextObj context = nullptr;
    CGLCreateContext(pixelFormat, mDisplayContext, &context);

    // Set up KVO to detect resizing.
    [self addObserver:self forKeyPath:@"bounds" options:0 context:nil];

    // Call OpenGL activation callback.
    CGLSetCurrentContext(context);
    int scaled_width = self.frame.size.width * self.contentsScale;
    int scaled_height = self.frame.size.height * self.contentsScale;
    appWindow->onOpenGLActivate(scaled_width, scaled_height);

    return context;
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
    CGLSetCurrentContext(mDisplayContext);

    float scaled_width = self.frame.size.width * self.contentsScale;
    float scaled_height = self.frame.size.height * self.contentsScale;
    appWindow->onResize(scaled_width, scaled_height);
}

@end
