#import "EditorView.h"
#import "base/buffer.h"
#import "ui/renderer/renderer.h"
#import "util/file_util.h"
#import "util/log_util.h"
#import <fstream>
#import <iostream>
#import <sstream>
#import <string>
#import <vector>

@interface OpenGLLayer : CAOpenGLLayer {
@public
    float x, y;
    CGPoint cursorPoint;
    CGPoint dragPoint;

    // @private
    Renderer* renderer;
    std::unique_ptr<Buffer> buffer;
    Rasterizer rasterizer;
}

- (void)insertUTF8String:(const char*)str bytes:(size_t)bytes;

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
        // openGLLayer.needsDisplayOnBoundsChange = true;
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
        // TODO: Formulate max_y without the need for division.
        float longest_line_x = openGLLayer->renderer->longest_line_x / openGLLayer.contentsScale;
        longest_line_x -= openGLLayer.frame.size.width;
        if (longest_line_x < 0) longest_line_x = 0;

        openGLLayer->x += event.scrollingDeltaX;
        if (-openGLLayer->x < 0) openGLLayer->x = 0;
        if (-openGLLayer->x > longest_line_x) openGLLayer->x = -longest_line_x;

        size_t line_count = openGLLayer->buffer->lineCount();
        line_count -= 1;  // TODO: Merge this with CursorRenderer.
        float max_y = line_count * openGLLayer->rasterizer.line_height;
        // TODO: Formulate max_y without the need for division.
        max_y /= openGLLayer.contentsScale;

        openGLLayer->y += event.scrollingDeltaY;

        if (-openGLLayer->y < 0) openGLLayer->y = 0;
        if (-openGLLayer->y > max_y) openGLLayer->y = -max_y;

        // https://developer.apple.com/documentation/appkit/nsevent/1527943-pressedmousebuttons?language=objc
        if (NSEvent.pressedMouseButtons & (1 << 0)) {
            // Prevent scrolling cursor past top of buffer.
            // FIXME: The behavior is still a little buggy near the top of buffer.
            if (!(openGLLayer->y == 0 && event.scrollingDeltaY > 0)) {
                openGLLayer->dragPoint.y -= event.scrollingDeltaY;

                float scroll_x = openGLLayer->x * openGLLayer.contentsScale;
                float scroll_y = openGLLayer->y * openGLLayer.contentsScale;
                float cursor_x = openGLLayer->cursorPoint.x * openGLLayer.contentsScale;
                float cursor_y = openGLLayer->cursorPoint.y * openGLLayer.contentsScale;
                float drag_x = openGLLayer->dragPoint.x * openGLLayer.contentsScale;
                float drag_y = openGLLayer->dragPoint.y * openGLLayer.contentsScale;
                openGLLayer->renderer->setCursorPositions(*openGLLayer->buffer, scroll_x, scroll_y,
                                                          cursor_x, cursor_y, drag_x, drag_y);
            }
        }

        [self.layer setNeedsDisplay];
    }
}

const char* hex(char c) {
    const char REF[] = "0123456789ABCDEF";
    static char output[3] = "XX";
    output[0] = REF[0x0f & c >> 4];
    output[1] = REF[0x0f & c];
    return output;
}

- (void)keyDown:(NSEvent*)event {
    const char* str = event.characters.UTF8String;
    size_t bytes = strlen(str);

    if (bytes > 0) {
        for (size_t i = 0; str[i] != '\0'; i++) {
            std::cout << hex(str[i]) << " ";
        }
        std::cout << '\n';

        if (str[0] == 0x0D) {
            std::cout << "new line inserted\n";
        }

        [openGLLayer insertUTF8String:str bytes:bytes];
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

    float scroll_x = openGLLayer->x * openGLLayer.contentsScale;
    float scroll_y = openGLLayer->y * openGLLayer.contentsScale;
    float cursor_x = openGLLayer->cursorPoint.x * openGLLayer.contentsScale;
    float cursor_y = openGLLayer->cursorPoint.y * openGLLayer.contentsScale;
    float drag_x = openGLLayer->dragPoint.x * openGLLayer.contentsScale;
    float drag_y = openGLLayer->dragPoint.y * openGLLayer.contentsScale;
    openGLLayer->renderer->setCursorPositions(*openGLLayer->buffer, scroll_x, scroll_y, cursor_x,
                                              cursor_y, drag_x, drag_y);
    [self.layer setNeedsDisplay];
}

- (void)mouseDragged:(NSEvent*)event {
    openGLLayer->dragPoint = event.locationInWindow;
    openGLLayer->dragPoint.x += openGLLayer->x;
    openGLLayer->dragPoint.y += openGLLayer->y;

    openGLLayer->dragPoint.y = openGLLayer.frame.size.height - openGLLayer->dragPoint.y;

    float scroll_x = openGLLayer->x * openGLLayer.contentsScale;
    float scroll_y = openGLLayer->y * openGLLayer.contentsScale;
    float cursor_x = openGLLayer->cursorPoint.x * openGLLayer.contentsScale;
    float cursor_y = openGLLayer->cursorPoint.y * openGLLayer.contentsScale;
    float drag_x = openGLLayer->dragPoint.x * openGLLayer.contentsScale;
    float drag_y = openGLLayer->dragPoint.y * openGLLayer.contentsScale;
    openGLLayer->renderer->setCursorPositions(*openGLLayer->buffer, scroll_x, scroll_y, cursor_x,
                                              cursor_y, drag_x, drag_y);
    [self.layer setNeedsDisplay];
}

- (void)rightMouseUp:(NSEvent*)event {
    NSMenu* contextMenu = [[NSMenu alloc] initWithTitle:@"Contextual Menu"];
    [contextMenu addItemWithTitle:@"Insert test string"
                           action:@selector(insertTestString)
                    keyEquivalent:@""];
    [contextMenu popUpMenuPositioningItem:nil atLocation:event.locationInWindow inView:self];
}

- (void)insertTestString {
    [openGLLayer insertUTF8String:u8"∆" bytes:3];
    [self.layer setNeedsDisplay];
}

// TODO: Implement light/dark mode detection.
- (void)viewDidChangeEffectiveAppearance {
    LogDefault("EditorView", "viewDidChangeEffectiveAppearance");
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
    CGLContextObj glContext = nullptr;
    CGLCreateContext(pixelFormat, nullptr, &glContext);
    if (glContext || (glContext = [super copyCGLContextForPixelFormat:pixelFormat])) {
        CGLSetCurrentContext(glContext);

        CGFloat fontSize = 16 * self.contentsScale;
        rasterizer = Rasterizer("Source Code Pro", fontSize);
        renderer = new Renderer(self.frame.size.width * self.contentsScale,
                                self.frame.size.height * self.contentsScale, "Source Code Pro",
                                fontSize, rasterizer.line_height);

        // std::ifstream infile(ResourcePath("sample_files/10k_lines.json"));
        // std::ifstream infile(ResourcePath("sample_files/larger_example.json"));
        // std::ifstream infile(ResourcePath("sample_files/example.json"));
        // std::ifstream infile(ResourcePath("sample_files/example.scm"));
        // std::ifstream infile(ResourcePath("sample_files/sort.scm"));
        // std::ifstream infile(ResourcePath("sample_files/sort_bugged.scm"));
        // std::ifstream infile(ResourcePath("sample_files/example.cc"));
        // std::ifstream infile(ResourcePath("sample_files/example.glsl"));
        // std::ifstream infile(ResourcePath("sample_files/strange.json"));
        std::ifstream infile(ResourcePath("sample_files/emojis.txt"));

        // buffer = std::unique_ptr<Buffer>(new Buffer("Hello world!\nthis is a new line"));
        buffer = std::unique_ptr<Buffer>(new Buffer(infile));

        uint64_t start = clock_gettime_nsec_np(CLOCK_MONOTONIC);
        renderer->parseBuffer(*buffer);
        uint64_t end = clock_gettime_nsec_np(CLOCK_MONOTONIC);
        uint64_t microseconds = (end - start) / 1e3;
        float fps = 1000000.0 / microseconds;
        LogDefault("OpenGLLayer", "Tree-sitter only parse: %ld µs (%f fps)", microseconds, fps);

        [self addObserver:self forKeyPath:@"bounds" options:0 context:nil];
    }
    return glContext;
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

    uint64_t start = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
    // [NSThread sleepForTimeInterval:0.02];  // Simulate lag.

    float scroll_x = x * self.contentsScale;
    float scroll_y = y * self.contentsScale;
    float width = self.frame.size.width * self.contentsScale;
    float height = self.frame.size.height * self.contentsScale;

    renderer->resize(width, height);
    renderer->renderText(*buffer, scroll_x, scroll_y);

    // Calls glFlush() by default.
    [super drawInCGLContext:glContext
                pixelFormat:pixelFormat
               forLayerTime:timeInterval
                displayTime:timeStamp];

    uint64_t end = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
    uint64_t microseconds = (end - start) / 1e3;
    float fps = 1000000.0 / microseconds;
    LogDefault(@"OpenGLLayer", @"%ld µs (%f fps)", microseconds, fps);
}

- (void)insertUTF8String:(const char*)str bytes:(size_t)bytes {
    (*buffer).data[renderer->cursor_end_line].insert(renderer->cursor_end_col_offset, str);

    uint64_t start = clock_gettime_nsec_np(CLOCK_MONOTONIC);
    renderer->editBuffer(*buffer, bytes);
    renderer->parseBuffer(*buffer);
    uint64_t end = clock_gettime_nsec_np(CLOCK_MONOTONIC);
    uint64_t microseconds = (end - start) / 1e3;
    float fps = 1000000.0 / microseconds;
    LogDefault("OpenGLLayer", "Tree-sitter edit and parse: %ld µs (%f fps)", microseconds, fps);

    float advance = renderer->getGlyphAdvance(std::string(str));
    renderer->cursor_start_col_offset += bytes;
    renderer->cursor_start_x += advance;
    renderer->cursor_end_col_offset += bytes;
    renderer->cursor_end_x += advance;
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context {
    [self setNeedsDisplay];
}

- (void)releaseCGLContext:(CGLContextObj)glContext {
    [super releaseCGLContext:glContext];
}

- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pixelFormat {
    [super releaseCGLPixelFormat:pixelFormat];
}

@end
