#import "EditorViewNew.h"
#import "base/buffer.h"
#import "base/syntax_highlighter.h"
#import "ui/renderer/text_renderer.h"

@interface EditorViewNew () {
    Buffer buffer;
    TextRenderer text_renderer;
    SyntaxHighlighter highlighter;
}
@end

@implementation EditorViewNew

- (CVReturn)getFrameForTime:(const CVTimeStamp*)outputTime {
    @autoreleasepool {
        [self drawView];
    }
    return kCVReturnSuccess;
}

static CVReturn MyDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now,
                                      const CVTimeStamp* outputTime, CVOptionFlags flagsIn,
                                      CVOptionFlags* flagsOut, void* displayLinkContext) {
    CVReturn result = [(__bridge EditorViewNew*)displayLinkContext getFrameForTime:outputTime];
    return result;
}

- (id)initWithFrame:(NSRect)frame {
    NSOpenGLPixelFormatAttribute attribs[] = {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAAllowOfflineRenderers,
        NSOpenGLPFAMultisample,
        1,
        NSOpenGLPFASampleBuffers,
        1,
        NSOpenGLPFASamples,
        4,
        NSOpenGLPFAColorSize,
        32,
        NSOpenGLPFADepthSize,
        32,
        NSOpenGLPFAOpenGLProfile,
        NSOpenGLProfileVersion3_2Core,
        0,
    };

    NSOpenGLPixelFormat* pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
    if (!pf) {
        NSLog(@"Failed to create pixel format.");
        return nil;
    }

    self = [super initWithFrame:frame pixelFormat:pf];
    if (self) {
        // This masks resizing glitches.
        // Solutions involving waiting result in throttled frame rate.
        // https://thume.ca/2019/06/19/glitchless-metal-window-resizing/
        // https://zed.dev/blog/120fps
        self.layerContentsPlacement = NSViewLayerContentsPlacementTopLeft;
    }
    return self;
}

- (void)initGL {
    [[self openGLContext] makeCurrentContext];

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    // Synchronize buffer swaps with vertical refresh rate
    GLint one = 1;
    [[self openGLContext] setValues:&one forParameter:NSOpenGLCPSwapInterval];
#pragma clang diagnostic pop

    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    int fontSize = 16;
    CGFloat scaledFontSize = [self convertSizeToBacking:CGSizeMake(fontSize, 0)].width;
    CGSize scaledSize = [self convertSizeToBacking:self.frame.size];

    text_renderer.setup(scaledSize.width, scaledSize.height, "Source Code Pro", scaledFontSize);
    highlighter.setLanguage("source.c++");
    buffer.setContents(ReadFile(ResourcePath() / "sample_files/text_renderer.cc"));
}

- (void)setupDisplayLink {
    CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);

    CVDisplayLinkSetOutputCallback(displayLink, &MyDisplayLinkCallback, (__bridge void*)self);

    CGLContextObj cglContext = [[self openGLContext] CGLContextObj];
    CGLPixelFormatObj cglPixelFormat = [[self pixelFormat] CGLPixelFormatObj];
    CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);

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

    // We draw on a secondary thread through the display link.
    // We lock to avoid the threads from accessing the context simultaneously.
    CGLLockContext([[self openGLContext] CGLContextObj]);

    CGSize scaledSize = [self convertSizeToBacking:self.frame.size];

    glViewport(0, 0, scaledSize.width, scaledSize.height);
    glClearColor(253 / 255.0, 253 / 255.0, 253 / 255.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    text_renderer.resize(scaledSize.width, scaledSize.height);
    text_renderer.renderText(0, 0, buffer, highlighter, 0, 0);

    [[self openGLContext] flushBuffer];

    CGLUnlockContext([[self openGLContext] CGLContextObj]);
}

- (void)dealloc {
    // Stop the display link BEFORE releasing anything in the view.
    // Otherwise, the display link thread may call into the view and crash when it encounters
    // something that has been released.
    CVDisplayLinkStop(displayLink);
    CVDisplayLinkRelease(displayLink);
}

@end
