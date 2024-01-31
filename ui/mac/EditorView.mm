#import "EditorView.h"
#import "base/buffer.h"
#import "ui/renderer/renderer.h"
#import "util/file_util.h"
#import "util/log_util.h"
#import <fstream>
#import <sstream>
#import <string>
#import <vector>

@interface OpenGLLayer : CAOpenGLLayer {
@public
    float x, y;
    CGPoint cursorPoint;
    CGPoint dragPoint;

@private
    Renderer* renderer;
    std::unique_ptr<Buffer> buffer;
}

- (void)insertCharacter:(char)ch;

@end

@interface EditorView () {
    OpenGLLayer* openGLLayer;
}
@end

@implementation EditorView

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        openGLLayer = [OpenGLLayer layer];
        openGLLayer.needsDisplayOnBoundsChange = true;
        // openGLLayer.asynchronous = true;
        self.layer = openGLLayer;

        // Fixes blurriness on HiDPI displays.
        // https://bugzilla.gnome.org/show_bug.cgi?id=765194
        self.layer.contentsScale = NSScreen.mainScreen.backingScaleFactor;

        NSTrackingAreaOptions options = NSTrackingMouseMoved | NSTrackingActiveInKeyWindow;
        trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds
                                                    options:options
                                                      owner:self
                                                   userInfo:nil];
        [self addTrackingArea:trackingArea];
    }
    return self;
}

- (void)updateTrackingAreas {
    [super updateTrackingAreas];
    [self removeTrackingArea:trackingArea];

    NSTrackingAreaOptions options = NSTrackingMouseMoved | NSTrackingActiveInKeyWindow;
    trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds
                                                options:options
                                                  owner:self
                                               userInfo:nil];
    [self addTrackingArea:trackingArea];
}

- (void)mouseMoved:(NSEvent*)event {
    [NSCursor.IBeamCursor set];
}

- (void)scrollWheel:(NSEvent*)event {
    if (event.type == NSEventTypeScrollWheel) {
        // openGLLayer->x += event.scrollingDeltaX;
        // if (openGLLayer->x > 0) openGLLayer->x = 0;

        openGLLayer->y += event.scrollingDeltaY;
        if (openGLLayer->y > 0) openGLLayer->y = 0;

        // https://developer.apple.com/documentation/appkit/nsevent/1527943-pressedmousebuttons?language=objc
        if (NSEvent.pressedMouseButtons & (1 << 0)) {
            // Prevent scrolling cursor past top of buffer.
            // FIXME: The behavior is still a little buggy near the top of buffer.
            if (!(openGLLayer->y == 0 && event.scrollingDeltaY > 0)) {
                openGLLayer->dragPoint.y -= event.scrollingDeltaY;
            }
        }

        [self.layer setNeedsDisplay];
    }
}

- (void)keyDown:(NSEvent*)event {
    NSString* characters = event.characters;
    for (uint32_t k = 0; k < characters.length; k++) {
        char ch = [characters characterAtIndex:k];
        LogDefault(@"WindowController", @"insert char: %c", ch);
        [openGLLayer insertCharacter:ch];
        // unichar key = [characters characterAtIndex:k];
        // switch (key) {
        // case 'k':
        //     openGLLayer->x = 0.0f;
        //     openGLLayer->y = 0.0f;
        //     [self.layer setNeedsDisplay];
        //     break;
        // }
        // openGLLayer->x = 0.0f;
        // openGLLayer->y = 0.0f;
        [self.layer setNeedsDisplay];
    }
}

- (void)mouseDown:(NSEvent*)event {
    openGLLayer->cursorPoint = event.locationInWindow;
    openGLLayer->cursorPoint.x += openGLLayer->x;
    openGLLayer->cursorPoint.y += openGLLayer->y;

    openGLLayer->dragPoint = event.locationInWindow;
    openGLLayer->dragPoint.x += openGLLayer->x;
    openGLLayer->dragPoint.y += openGLLayer->y;

    openGLLayer->cursorPoint.y = openGLLayer.frame.size.height - openGLLayer->cursorPoint.y;
    openGLLayer->dragPoint.y = openGLLayer.frame.size.height - openGLLayer->dragPoint.y;

    [self.layer setNeedsDisplay];
}

- (void)mouseDragged:(NSEvent*)event {
    openGLLayer->dragPoint = event.locationInWindow;
    openGLLayer->dragPoint.x += openGLLayer->x;
    openGLLayer->dragPoint.y += openGLLayer->y;

    openGLLayer->dragPoint.y = openGLLayer.frame.size.height - openGLLayer->dragPoint.y;

    [self.layer setNeedsDisplay];
}

- (void)rightMouseUp:(NSEvent*)event {
    NSMenu* contextMenu = [[NSMenu alloc] initWithTitle:@"Contextual Menu"];
    [contextMenu addItemWithTitle:@"Insert test string"
                           action:@selector(insertTestString)
                    keyEquivalent:@""];
    [contextMenu popUpMenuPositioningItem:nil atLocation:NSEvent.mouseLocation inView:self];
}

- (void)insertTestString {
    [openGLLayer insertCharacter:'h'];
    [openGLLayer insertCharacter:'e'];
    [openGLLayer insertCharacter:'l'];
    [openGLLayer insertCharacter:'l'];
    [openGLLayer insertCharacter:'o'];
    [self.layer setNeedsDisplay];
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
        renderer = new Renderer(self.frame.size.width * self.contentsScale,
                                self.frame.size.height * self.contentsScale, "Source Code Pro",
                                "Apple Color Emoji", fontSize * self.contentsScale);

        std::ifstream infile(ResourcePath("sample_files/larger_example.json"));

        // buffer = std::unique_ptr<Buffer>(new Buffer("Hello world!\nthis is a new line"));
        buffer = std::unique_ptr<Buffer>(new Buffer(infile));

        // for (const std::string& line : *buffer) {
        //     LogDefault("EditorView", line);
        // }
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

    uint64_t start = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
    // [NSThread sleepForTimeInterval:0.02];  // Simulate lag.

    float scroll_x = x * self.contentsScale;
    float scroll_y = y * self.contentsScale;
    float cursor_x = cursorPoint.x * self.contentsScale;
    float cursor_y = cursorPoint.y * self.contentsScale;
    float drag_x = dragPoint.x * self.contentsScale;
    float drag_y = dragPoint.y * self.contentsScale;
    float width = self.frame.size.width * self.contentsScale;
    float height = self.frame.size.height * self.contentsScale;

    renderer->resize(width, height);
    renderer->setCursorPositions(*buffer, cursor_x, cursor_y, drag_x, drag_y);
    renderer->renderText(*buffer, scroll_x, scroll_y);

    // Calls glFlush() by default.
    [super drawInCGLContext:context
                pixelFormat:pixelFormat
               forLayerTime:timeInterval
                displayTime:timeStamp];

    uint64_t end = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
    uint64_t microseconds = (end - start) / 1e3;
    float fps = 1000000.0 / microseconds;
    LogDefault(@"OpenGLLayer", @"%ld µs (%f fps)", microseconds, fps);
}

- (void)insertCharacter:(char)ch {
    (*buffer).data[renderer->drag_cursor_row].insert(renderer->drag_cursor_byte_offset, 1, ch);
}

- (void)releaseCGLContext:(CGLContextObj)context {
    [super releaseCGLContext:context];
}

- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pixelFormat {
    [super releaseCGLPixelFormat:pixelFormat];
}

@end
