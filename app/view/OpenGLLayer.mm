#import "OpenGLLayer.h"
#import "model/Renderer.h"
#import "util/FileUtil.h"
#import "util/LogUtil.h"
#include <fstream>
#import <sstream>
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

        CGFloat fontSize = 16;
        renderer =
            new Renderer(NSScreen.mainScreen.frame.size.width * self.contentsScale,
                         NSScreen.mainScreen.frame.size.height * self.contentsScale,
                         "Source Code Pro", "Apple Color Emoji", fontSize * self.contentsScale);

        // x = 500.0f;
        // y = 500.0f;

        std::ifstream infile(ResourcePath("larger_example.json"));
        std::string line;
        while (std::getline(infile, line)) {
            text.push_back(line);
        }
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

    uint64_t start = clock_gettime_nsec_np(CLOCK_MONOTONIC);
    // [NSThread sleepForTimeInterval:0.02];  // Simulate lag.

    renderer->clearAndResize();

    // renderer->renderText(std::to_string(timeInterval), x, y);

    uint16_t row_offset = round(y / -42.0);
    renderer->renderText(text, x, y, row_offset);

    // Calls glFlush() by default.
    [super drawInCGLContext:context
                pixelFormat:pixelFormat
               forLayerTime:timeInterval
                displayTime:timeStamp];

    uint64_t end = clock_gettime_nsec_np(CLOCK_MONOTONIC);
    uint64_t microseconds = (end - start) / 1e3;
    float fps = 1000000.0 / microseconds;
    LogDefault(@"OpenGLLayer", @"%ld µs (%f fps)", microseconds, fps);
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
