#import "OpenGLLayer.h"
#import "model/Atlas.h"
#import "model/Font.h"
#import "model/Rasterizer.h"
#import "model/Renderer.h"
#import "util/FileUtil.h"
#import "util/LogUtil.h"
#import <OpenGL/gl3.h>
#import <glm/glm.hpp>
#import <glm/gtc/matrix_transform.hpp>
#import <glm/gtc/type_ptr.hpp>

@interface OpenGLLayer () {
    GLfloat screenWidth, screenHeight;
    Renderer* renderer;
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
        screenWidth = NSScreen.mainScreen.frame.size.width;
        screenHeight = NSScreen.mainScreen.frame.size.height;

        // https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/opengl_designstrategies/opengl_designstrategies.html#//apple_ref/doc/uid/TP40001987-CH2-SW4
        // GLint params = 1;
        // CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &params);

        CGLSetCurrentContext(context);

        // Font menlo = Font(CFSTR("Menlo"), 32);
        // Metrics metrics = menlo.metrics();
        // logDefault(@"OpenGL", @"average_advance = %f, line_height = %f",
        // metrics.average_advance,
        //            metrics.line_height);

        // float cell_width = floor(metrics.average_advance + 1);
        // float cell_height = floor(metrics.line_height + 2);
        // logDefault(@"OpenGL", @"cell_width = %f, cell_height = %f", cell_width, cell_height);

        // Rasterizer rasterizer = Rasterizer();
        // CGGlyph glyph_index = menlo.get_glyph(@"E");
        // RasterizedGlyph rasterized_glyph = rasterizer.rasterize_glyph(glyph_index);

        // Atlas atlas = Atlas(1024);
        // Glyph glyph = atlas.insert_inner(rasterized_glyph);

        // logDefault(@"OpenGL", @"%s", glGetString(GL_VERSION));
        // logDefault(@"OpenGL", @"%fx%f", screenWidth, screenHeight);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        renderer = new Renderer(screenWidth, screenHeight);
        renderer->init();

        [self draw];  // Initial draw call.
    }
    return context;
}

- (void)draw {
    glViewport(0, 0, screenWidth, screenHeight);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    renderer->render_text("This is sample text", 25.0f, 25.0f, 1.0f, glm::vec3(0.5, 0.8f, 0.2f));
    renderer->render_text("This is sample text", 540.0f, 570.0f, 0.5f, glm::vec3(0.3, 0.7f, 0.9f));
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

    [self draw];

    // Calls glFlush() by default.
    [super drawInCGLContext:glContext
                pixelFormat:pixelFormat
               forLayerTime:timeInterval
                displayTime:timeStamp];
}

- (void)releaseCGLContext:(CGLContextObj)glContext {
    [super releaseCGLContext:glContext];
}

- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pixelFormat {
    [super releaseCGLPixelFormat:pixelFormat];
}

@end
