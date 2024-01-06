#import "MyNSOpenGLView.h"
#import "model/BoingRenderer.h"
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl3.h>

@interface MyNSOpenGLView () {
    NSOpenGLPixelFormat* pixelFormat;
    BOOL enableMultisample;
    CVDisplayLinkRef displayLink;
    BoingRenderer* renderer;

    CGFloat width, height;
}
@end

@implementation MyNSOpenGLView

- (CVReturn)getFrameForTime:(const CVTimeStamp*)outputTime {
    // There is no autorelease pool when this method is called
    // because it will be called from a background thread
    // It's important to create one or you will leak objects
    @autoreleasepool {
        [self drawView];
    }
    return kCVReturnSuccess;
}

static CVReturn MyDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now,
                                      const CVTimeStamp* outputTime, CVOptionFlags flagsIn,
                                      CVOptionFlags* flagsOut, void* displayLinkContext) {
    CVReturn result = [(__bridge MyNSOpenGLView*)displayLinkContext getFrameForTime:outputTime];
    return result;
}

- (id)initWithFrame:(NSRect)frame {
    // NOTE: to use integrated GPU:
    // 1. NSOpenGLPFAAllowOfflineRenderers when using NSOpenGL
    // 2. kCGLPFAAllowOfflineRenderers when using CGL
    NSOpenGLPixelFormatAttribute attribs[] = {NSOpenGLPFAColorSize,
                                              24,
                                              NSOpenGLPFADoubleBuffer,
                                              NSOpenGLPFAAllowOfflineRenderers,
                                              NSOpenGLPFAOpenGLProfile,
                                              NSOpenGLProfileVersion3_2Core,
                                              0};

    NSOpenGLPixelFormat* pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
    if (!pf) {
        NSLog(@"Failed to create pixel format.");
        return nil;
    }

    self = [super initWithFrame:frame pixelFormat:pf];
    if (self) {
        pixelFormat = pf;
        enableMultisample = YES;
        width = self.bounds.size.width * 2;
        height = self.bounds.size.height * 2;
    }

    return self;
}

- (void)initGL {
    [[self openGLContext] makeCurrentContext];

    // Synchronize buffer swaps with vertical refresh rate
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    GLint one = 1;
    [[self openGLContext] setValues:&one forParameter:NSOpenGLCPSwapInterval];
#pragma clang diagnostic pop

    if (enableMultisample) glEnable(GL_MULTISAMPLE);
}

- (void)setupDisplayLink {
    // Create a display link capable of being used with all active displays
    CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);

    // Set the renderer output callback function
    CVDisplayLinkSetOutputCallback(displayLink, &MyDisplayLinkCallback, (__bridge void*)self);

    // Set the display link for the current renderer
    CGLContextObj cglContext = [[self openGLContext] CGLContextObj];
    CGLPixelFormatObj cglPixelFormat = [[self pixelFormat] CGLPixelFormatObj];
    CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);

    // Activate the display link
    CVDisplayLinkStart(displayLink);
}

- (void)prepareOpenGL {
    [super prepareOpenGL];
    [self initGL];
    [self setupDisplayLink];
}

- (void)update {
    [super update];
    [[self openGLContext] update];
}

- (void)drawView {
    [[self openGLContext] makeCurrentContext];

    // We draw on a secondary thread through the display link
    // Add a mutex around to avoid the threads from accessing the context
    // simultaneously
    CGLLockContext([[self openGLContext] CGLContextObj]);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!renderer) {
        renderer = [[BoingRenderer alloc] init];
        [renderer makeOrthographicForWidth:width * 2 height:height * 2];
    }

    [renderer updateForWidth:width height:height];
    [renderer render];

    [[self openGLContext] flushBuffer];

    CGLUnlockContext([[self openGLContext] CGLContextObj]);
}

- (void)dealloc {
    // Stop the display link BEFORE releasing anything in the view
    // otherwise the display link thread may call into the view and crash
    // when it encounters something that has been released
    CVDisplayLinkStop(displayLink);

    CVDisplayLinkRelease(displayLink);
}

@end
