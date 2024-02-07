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
    CGFloat scroll_x;
    CGFloat scroll_y;
    CGPoint cursorPoint;
    CGPoint dragPoint;

    // @private
    Renderer* renderer;
    Buffer buffer;
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

        openGLLayer->scroll_x -= event.scrollingDeltaX;
        if (openGLLayer->scroll_x < 0) openGLLayer->scroll_x = 0;
        if (openGLLayer->scroll_x > longest_line_x) openGLLayer->scroll_x = longest_line_x;

        size_t line_count = openGLLayer->buffer.lineCount();
        line_count -= 1;  // TODO: Merge this with CursorRenderer.
        float max_y = line_count * openGLLayer->rasterizer.line_height;
        // TODO: Formulate max_y without the need for division.
        max_y /= openGLLayer.contentsScale;

        openGLLayer->scroll_y -= event.scrollingDeltaY;
        if (openGLLayer->scroll_y < 0) openGLLayer->scroll_y = 0;
        if (openGLLayer->scroll_y > max_y) openGLLayer->scroll_y = max_y;

        // https://developer.apple.com/documentation/appkit/nsevent/1527943-pressedmousebuttons?language=objc
        // if (NSEvent.pressedMouseButtons & (1 << 0)) {
        //     // Prevent scrolling cursor past top of buffer.
        //     // FIXME: The behavior is still a little buggy near the top of buffer.
        //     if (!(openGLLayer->scroll_y == 0 && event.scrollingDeltaY > 0)) {
        //         openGLLayer->dragPoint.y -= event.scrollingDeltaY;

        //         float scroll_x = openGLLayer->scroll_x * openGLLayer.contentsScale;
        //         float scroll_y = openGLLayer->scroll_y * openGLLayer.contentsScale;
        //         float cursor_x = openGLLayer->cursorPoint.x * openGLLayer.contentsScale;
        //         float cursor_y = openGLLayer->cursorPoint.y * openGLLayer.contentsScale;
        //         float drag_x = openGLLayer->dragPoint.x * openGLLayer.contentsScale;
        //         float drag_y = openGLLayer->dragPoint.y * openGLLayer.contentsScale;
        //         openGLLayer->renderer->setCursorPositions(openGLLayer->buffer, scroll_x,
        //         scroll_y,
        //                                                   cursor_x, cursor_y, drag_x, drag_y);
        //     }
        // }

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
    // openGLLayer->cursorPoint = event.locationInWindow;
    // openGLLayer->cursorPoint.x += openGLLayer->scroll_x;
    // openGLLayer->cursorPoint.y += openGLLayer->scroll_y;

    // openGLLayer->dragPoint = event.locationInWindow;
    // openGLLayer->dragPoint.x += openGLLayer->scroll_x;
    // openGLLayer->dragPoint.y += openGLLayer->scroll_y;

    // openGLLayer->cursorPoint.y = openGLLayer.frame.size.height - openGLLayer->cursorPoint.y;
    // openGLLayer->dragPoint.y = openGLLayer.frame.size.height - openGLLayer->dragPoint.y;

    // float scaled_scroll_x = openGLLayer->scroll_x * openGLLayer.contentsScale;
    // float scaled_scroll_y = openGLLayer->scroll_y * openGLLayer.contentsScale;
    // float scaled_cursor_x = openGLLayer->cursorPoint.x * openGLLayer.contentsScale;
    // float scaled_cursor_y = openGLLayer->cursorPoint.y * openGLLayer.contentsScale;
    // float scaled_drag_x = openGLLayer->dragPoint.x * openGLLayer.contentsScale;
    // float scaled_drag_y = openGLLayer->dragPoint.y * openGLLayer.contentsScale;
    // openGLLayer->renderer->setCursorPositions(openGLLayer->buffer, scaled_scroll_x,
    //                                           scaled_scroll_y, scaled_cursor_x, scaled_cursor_y,
    //                                           scaled_drag_x, scaled_drag_y);

    CGFloat mouse_x = event.locationInWindow.x;
    CGFloat mouse_y = event.locationInWindow.y;

    // Set origin at top left.
    mouse_y = openGLLayer.frame.size.height - mouse_y;

    CGFloat scroll_x = openGLLayer->scroll_x;
    CGFloat scroll_y = openGLLayer->scroll_y;
    CGFloat cursor_x = mouse_x + scroll_x;
    CGFloat cursor_y = mouse_y + scroll_y;

    scroll_x *= openGLLayer.contentsScale;
    scroll_y *= openGLLayer.contentsScale;
    cursor_x *= openGLLayer.contentsScale;
    cursor_y *= openGLLayer.contentsScale;

    LogDefault("EditorView", "width = %f, height = %f", openGLLayer.frame.size.width,
               openGLLayer.frame.size.height);

    openGLLayer->renderer->setCursorPositions(openGLLayer->buffer, scroll_x, scroll_y, cursor_x,
                                              cursor_y, cursor_x, cursor_y);
    [self.layer setNeedsDisplay];

    LogDefault("EditorView", "mouse_x = %f, mouse_y = %f", mouse_x, mouse_y);
    LogDefault("EditorView", "scroll_x = %f, scroll_y = %f", openGLLayer->scroll_x,
               openGLLayer->scroll_y);
}

- (void)mouseDragged:(NSEvent*)event {
    // openGLLayer->dragPoint = event.locationInWindow;
    // openGLLayer->dragPoint.x += openGLLayer->scroll_x;
    // openGLLayer->dragPoint.y += openGLLayer->scroll_y;

    // openGLLayer->dragPoint.y = openGLLayer.frame.size.height - openGLLayer->dragPoint.y;

    // float scroll_x = openGLLayer->scroll_x * openGLLayer.contentsScale;
    // float scroll_y = openGLLayer->scroll_y * openGLLayer.contentsScale;
    // float cursor_x = openGLLayer->cursorPoint.x * openGLLayer.contentsScale;
    // float cursor_y = openGLLayer->cursorPoint.y * openGLLayer.contentsScale;
    // float drag_x = openGLLayer->dragPoint.x * openGLLayer.contentsScale;
    // float drag_y = openGLLayer->dragPoint.y * openGLLayer.contentsScale;
    // openGLLayer->renderer->setCursorPositions(openGLLayer->buffer, scroll_x, scroll_y, cursor_x,
    //                                           cursor_y, drag_x, drag_y);
    // [self.layer setNeedsDisplay];
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
        std::ifstream infile(ResourcePath("sample_files/sort.scm"));
        // std::ifstream infile(ResourcePath("sample_files/sort_bugged.scm"));
        // std::ifstream infile(ResourcePath("sample_files/example.cc"));
        // std::ifstream infile(ResourcePath("sample_files/example.glsl"));
        // std::ifstream infile(ResourcePath("sample_files/strange.json"));
        // std::ifstream infile(ResourcePath("sample_files/emojis.txt"));

        // buffer = std::unique_ptr<Buffer>(new Buffer("Hello world!\nthis is a new line"));
        // buffer = std::unique_ptr<Buffer>(new Buffer(infile));
        buffer.setContents(infile);

        uint64_t start = clock_gettime_nsec_np(CLOCK_MONOTONIC);
        renderer->parseBuffer(buffer);
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

    float scaled_scroll_x = scroll_x * self.contentsScale;
    float scaled_scroll_y = scroll_y * self.contentsScale;
    float width = self.frame.size.width * self.contentsScale;
    float height = self.frame.size.height * self.contentsScale;

    renderer->resize(width, height);
    renderer->renderText(buffer, scaled_scroll_x, scaled_scroll_y);

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
    buffer.insert(renderer->cursor_end_line, renderer->cursor_end_col_offset, str);

    uint64_t start = clock_gettime_nsec_np(CLOCK_MONOTONIC);
    renderer->editBuffer(buffer, bytes);
    renderer->parseBuffer(buffer);
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
    // // TODO: Formulate max_y without the need for division.
    // float longest_line_x = renderer->longest_line_x / self.contentsScale;
    // longest_line_x -= self.frame.size.width;
    // if (longest_line_x < 0) longest_line_x = 0;

    // if (-x < 0) x = 0;
    // if (-x > longest_line_x) x = -longest_line_x;

    // size_t line_count = buffer.lineCount();
    // line_count -= 1;  // TODO: Merge this with CursorRenderer.
    // float max_y = line_count * rasterizer.line_height;
    // // TODO: Formulate max_y without the need for division.
    // max_y /= self.contentsScale;

    // if (-y < 0) y = 0;
    // if (-y > max_y) y = -max_y;

    // float scroll_x = x * self.contentsScale;
    // float scroll_y = y * self.contentsScale;
    // float cursor_x = cursorPoint.x * self.contentsScale;
    // float cursor_y = cursorPoint.y * self.contentsScale;
    // float drag_x = dragPoint.x * self.contentsScale;
    // float drag_y = dragPoint.y * self.contentsScale;
    // renderer->setCursorPositions(buffer, scroll_x, scroll_y, cursor_x, cursor_y, drag_x,
    // drag_y);
    [self setNeedsDisplay];
}

- (void)releaseCGLContext:(CGLContextObj)glContext {
    [super releaseCGLContext:glContext];
}

- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pixelFormat {
    [super releaseCGLPixelFormat:pixelFormat];
}

@end
