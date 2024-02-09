#import "EditorView.h"
#import "base/buffer.h"
#import "ui/renderer/rect_renderer.h"
#import "ui/renderer/text_renderer.h"
#import "util/file_util.h"
#import "util/log_util_mac.h"
#import <fstream>
#import <iostream>
#import <limits>
#import <sstream>
#import <string>
#import <vector>

@interface OpenGLLayer : CAOpenGLLayer {
@public
    CGFloat scroll_x;
    CGFloat scroll_y;
    CGFloat cursor_start_x;
    CGFloat cursor_start_y;
    CGFloat cursor_end_x;
    CGFloat cursor_end_y;

    // @private
    TextRenderer text_renderer;
    Buffer buffer;

@private
    RectRenderer rect_renderer;
}

- (void)insertUTF8String:(const char*)str bytes:(size_t)bytes;

- (void)setRendererCursorPositions;

- (CGFloat)maxCursorX;

- (CGFloat)maxCursorY;

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
        CGFloat dx = -event.scrollingDeltaX;
        CGFloat dy = -event.scrollingDeltaY;

        openGLLayer->scroll_x =
            std::clamp(openGLLayer->scroll_x + dx, 0.0, [openGLLayer maxCursorX]);
        openGLLayer->scroll_y =
            std::clamp(openGLLayer->scroll_y + dy, 0.0, [openGLLayer maxCursorY]);

        // https://developer.apple.com/documentation/appkit/nsevent/1527943-pressedmousebuttons?language=objc
        if (NSEvent.pressedMouseButtons & (1 << 0)) {
            CGFloat mouse_x = event.locationInWindow.x;
            CGFloat mouse_y = event.locationInWindow.y;
            mouse_y = openGLLayer.frame.size.height - mouse_y;  // Set origin at top left.

            mouse_x -= 200;
            mouse_y -= 30;

            CGFloat max_mouse_x =
                (openGLLayer->text_renderer.longest_line_x / openGLLayer.contentsScale) -
                (openGLLayer.frame.size.width - mouse_x);
            CGFloat max_mouse_y =
                (openGLLayer->buffer.lineCount() * openGLLayer->text_renderer.line_height) /
                openGLLayer.contentsScale;

            openGLLayer->cursor_end_x =
                std::clamp(openGLLayer->cursor_end_x + dx, mouse_x, max_mouse_x);

            // Unlike cursor_end_x, our mouse could be well below the cursor.
            // This is only possible if scrolling past the end is enabled.
            // For upwards scrolling, don't actually scroll the cursor until our mouse meets it.
            if (!(dy < 0 && mouse_y + openGLLayer->scroll_y > openGLLayer->cursor_end_y)) {
                openGLLayer->cursor_end_y =
                    std::clamp(openGLLayer->cursor_end_y + dy, mouse_y, max_mouse_y);
            }

            [openGLLayer setRendererCursorPositions];
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
    CGFloat mouse_x = event.locationInWindow.x;
    CGFloat mouse_y = event.locationInWindow.y;
    mouse_y = openGLLayer.frame.size.height - mouse_y;  // Set origin at top left.

    mouse_x -= 200;
    mouse_y -= 30;

    openGLLayer->cursor_start_x = mouse_x + openGLLayer->scroll_x;
    openGLLayer->cursor_start_y = mouse_y + openGLLayer->scroll_y;
    openGLLayer->cursor_end_x = mouse_x + openGLLayer->scroll_x;
    openGLLayer->cursor_end_y = mouse_y + openGLLayer->scroll_y;
    [openGLLayer setRendererCursorPositions];
}

- (void)mouseDragged:(NSEvent*)event {
    CGFloat mouse_x = event.locationInWindow.x;
    CGFloat mouse_y = event.locationInWindow.y;
    mouse_y = openGLLayer.frame.size.height - mouse_y;  // Set origin at top left.

    mouse_x -= 200;
    mouse_y -= 30;

    openGLLayer->cursor_end_x = mouse_x + openGLLayer->scroll_x;
    openGLLayer->cursor_end_y = mouse_y + openGLLayer->scroll_y;
    [openGLLayer setRendererCursorPositions];
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
        float scaled_width = self.frame.size.width * self.contentsScale;
        float scaled_height = self.frame.size.height * self.contentsScale;
        text_renderer.setup(scaled_width, scaled_height, "Source Code Pro", fontSize);
        rect_renderer.setup(scaled_width, scaled_height);

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
        text_renderer.parseBuffer(buffer);
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

    text_renderer.resize(width, height);
    text_renderer.renderText(buffer, scaled_scroll_x, scaled_scroll_y);

    size_t visible_lines = std::ceil((height - 60 - 40) / text_renderer.line_height);
    rect_renderer.resize(width, height);
    glDisable(GL_BLEND);
    rect_renderer.draw(scaled_scroll_x, scaled_scroll_y, text_renderer.cursor_end_x,
                       text_renderer.cursor_end_line, text_renderer.line_height,
                       buffer.lineCount(), text_renderer.longest_line_x, visible_lines);
    glEnable(GL_BLEND);

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
    buffer.insert(text_renderer.cursor_end_line, text_renderer.cursor_end_col_offset, str);

    uint64_t start = clock_gettime_nsec_np(CLOCK_MONOTONIC);
    text_renderer.editBuffer(buffer, bytes);
    text_renderer.parseBuffer(buffer);
    uint64_t end = clock_gettime_nsec_np(CLOCK_MONOTONIC);
    uint64_t microseconds = (end - start) / 1e3;
    float fps = 1000000.0 / microseconds;
    LogDefault("OpenGLLayer", "Tree-sitter edit and parse: %ld µs (%f fps)", microseconds, fps);

    float advance = text_renderer.getGlyphAdvance(std::string(str));
    text_renderer.cursor_start_col_offset += bytes;
    text_renderer.cursor_start_x += advance;
    text_renderer.cursor_end_col_offset += bytes;
    text_renderer.cursor_end_x += advance;
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context {
    scroll_x = std::clamp(scroll_x, 0.0, [self maxCursorX]);
    scroll_y = std::clamp(scroll_y, 0.0, [self maxCursorY]);
    [self setNeedsDisplay];
}

- (void)setRendererCursorPositions {
    CGFloat scale = self.contentsScale;
    text_renderer.setCursorPositions(buffer, cursor_start_x * scale, cursor_start_y * scale,
                                     cursor_end_x * scale, cursor_end_y * scale);
    [self setNeedsDisplay];
}

- (CGFloat)maxCursorX {
    // TODO: Formulate max_cursor_x without the need for division.
    CGFloat max_cursor_x = text_renderer.longest_line_x / self.contentsScale;
    max_cursor_x -= self.frame.size.width;
    if (max_cursor_x < 0) max_cursor_x = 0;
    return max_cursor_x;
}

- (CGFloat)maxCursorY {
    size_t line_count = buffer.lineCount();
    line_count -= 1;  // TODO: Merge this with RectRenderer.
    CGFloat max_y = line_count * text_renderer.line_height;
    // TODO: Formulate max_y without the need for division.
    max_y /= self.contentsScale;
    return max_y;
}

- (void)releaseCGLContext:(CGLContextObj)glContext {
    [super releaseCGLContext:glContext];
}

- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pixelFormat {
    [super releaseCGLPixelFormat:pixelFormat];
}

@end
