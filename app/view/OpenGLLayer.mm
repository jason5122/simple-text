#import "OpenGLLayer.h"
#import "model/Renderer.h"
#import <string>
#import <vector>

@interface OpenGLLayer () {
    Renderer* renderer;
    std::vector<std::string> text;
}
@end

@implementation OpenGLLayer

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
    CGLPixelFormatAttribute attribs[] = {
        kCGLPFADisplayMask,
        static_cast<CGLPixelFormatAttribute>(mask),
        kCGLPFAColorSize,
        static_cast<CGLPixelFormatAttribute>(24),
        kCGLPFAAlphaSize,
        static_cast<CGLPixelFormatAttribute>(8),
        kCGLPFAAccelerated,
        kCGLPFANoRecovery,
        kCGLPFADoubleBuffer,
        kCGLPFAAllowOfflineRenderers,
        kCGLPFAOpenGLProfile,
        static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_3_2_Core),
        static_cast<CGLPixelFormatAttribute>(0),
    };

    CGLPixelFormatObj pixelFormat = nullptr;
    GLint numFormats = 0;
    CGLChoosePixelFormat(attribs, &pixelFormat, &numFormats);
    return pixelFormat;
}

- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat {
    CGLContextObj context = nullptr;
    CGLCreateContext(pixelFormat, nullptr, &context);
    if (context || (context = [super copyCGLContextForPixelFormat:pixelFormat])) {
        CGLSetCurrentContext(context);

        CGFloat fontSize = 48;
        CTFontRef mainFont =
            CTFontCreateWithName(CFSTR("Source Code Pro"), fontSize * self.contentsScale, nullptr);
        renderer =
            new Renderer(NSScreen.mainScreen.frame.size.width * self.contentsScale,
                         NSScreen.mainScreen.frame.size.height * self.contentsScale, mainFont);

        x = 500.0f;
        y = 500.0f;
        text.push_back("");
        text.push_back("hi there");
    }
    return context;
}

- (BOOL)canDrawInCGLContext:(CGLContextObj)context
                pixelFormat:(CGLPixelFormatObj)pixelFormat
               forLayerTime:(CFTimeInterval)timeInterval
                displayTime:(const CVTimeStamp*)timeStamp {
    return true;
}

- (void)drawInCGLContext:(CGLContextObj)context
             pixelFormat:(CGLPixelFormatObj)pixelFormat
            forLayerTime:(CFTimeInterval)timeInterval
             displayTime:(const CVTimeStamp*)timeStamp {
    CGLSetCurrentContext(context);

    renderer->clearAndResize();

    // renderer->renderText(std::to_string(timeInterval), x, y);
    // renderer->renderText("Hello world!", x, y);
    // renderer->renderText("Hello world! -", x, y);

    renderer->renderText(text, x, y);

    // Calls glFlush() by default.
    [super drawInCGLContext:context
                pixelFormat:pixelFormat
               forLayerTime:timeInterval
                displayTime:timeStamp];
}

- (void)insertCharacter:(char)ch {
    text[0].push_back(ch);
}

- (void)releaseCGLContext:(CGLContextObj)context {
    [super releaseCGLContext:context];
}

- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pixelFormat {
    [super releaseCGLPixelFormat:pixelFormat];
}

@end
