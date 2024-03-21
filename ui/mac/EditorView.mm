#import "EditorView.h"
#import "base/buffer.h"
#import "base/syntax_highlighter.h"
#import "ui/renderer/image_renderer.h"
#import "ui/renderer/rect_renderer.h"
#import "ui/renderer/text_renderer.h"
#import "util/file_util.h"
#import "util/profile_util.h"
#import <chrono>
#import <fstream>
#import <iostream>
#import <limits>
#import <sstream>
#import <string>
#import <thread>
#import <vector>

@interface OpenGLLayer : CAOpenGLLayer {
@public
    CGFloat scroll_x;
    CGFloat scroll_y;
    CGFloat cursor_start_x;
    CGFloat cursor_start_y;
    CGFloat cursor_end_x;
    CGFloat cursor_end_y;

    float editor_offset_x;
    float editor_offset_y;

    // @private
    TextRenderer text_renderer;
    Buffer buffer;

@private
    RectRenderer rect_renderer;
    ImageRenderer image_renderer;
    SyntaxHighlighter highlighter;

    bool isSideBarExpanding;
}

- (void)insertUTF8String:(const char*)str bytes:(size_t)bytes;

- (void)removeBytes:(size_t)bytes;

- (void)backspaceBytes:(size_t)bytes;

- (void)parseBuffer;

- (void)editBuffer:(size_t)bytes;

- (void)setRendererCursorPositions;

- (CGFloat)maxScrollX;

- (CGFloat)maxScrollY;

@end

@interface EditorView () {
@public
    OpenGLLayer* openGLLayer;

@private
    bool isDragging;
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

        // This masks resizing glitches.
        // Solutions involving waiting result in throttled frame rate.
        // https://thume.ca/2019/06/19/glitchless-metal-window-resizing/
        // https://zed.dev/blog/120fps
        self.layerContentsPlacement = NSViewLayerContentsPlacementTopLeft;

        NSTrackingAreaOptions options =
            NSTrackingMouseMoved | NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow;
        trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds
                                                    options:options
                                                      owner:self
                                                   userInfo:nil];
        [self addTrackingArea:trackingArea];

        isDragging = false;
    }
    return self;
}

- (void)updateTrackingAreas {
    [super updateTrackingAreas];
    [self removeTrackingArea:trackingArea];

    NSTrackingAreaOptions options =
        NSTrackingMouseMoved | NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow;
    trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds
                                                options:options
                                                  owner:self
                                               userInfo:nil];
    [self addTrackingArea:trackingArea];
}

// FIXME: Also set cursor style when clicking window to focus.
// This can be reproduced by opening another window in front of this one, and clicking on this
// without moving the mouse.
- (void)setCursorStyle:(NSEvent*)event {
    CGFloat mouse_x = event.locationInWindow.x;
    CGFloat mouse_y = event.locationInWindow.y;
    mouse_y = openGLLayer.frame.size.height - mouse_y;  // Set origin at top left.

    if (mouse_x < openGLLayer->editor_offset_x) {
        [NSCursor.resizeLeftRightCursor set];
    } else if (mouse_y < openGLLayer->editor_offset_y) {
        [NSCursor.arrowCursor set];
    } else {
        [NSCursor.IBeamCursor set];
    }
}

- (void)mouseMoved:(NSEvent*)event {
    [self setCursorStyle:event];
}

- (void)mouseEntered:(NSEvent*)event {
    [self setCursorStyle:event];
}

// We need to override `cursorUpdate` to stop the event from being passed up in the chain.
// Without this, our `mouseEntered` NSCursor set will be overridden.
// https://stackoverflow.com/a/20197686
- (void)cursorUpdate:(NSEvent*)event {
}

- (void)mouseExited:(NSEvent*)event {
    // TODO: Don't reset when cursor exits window while drag selecting text.
    // [NSCursor.arrowCursor set];
}

- (void)scrollWheel:(NSEvent*)event {
    if (event.type == NSEventTypeScrollWheel) {
        if (event.momentumPhase & NSEventPhaseBegan) {
            openGLLayer.asynchronous = true;
        }
        if (event.momentumPhase & NSEventPhaseEnded) {
            // openGLLayer.asynchronous = false;
        }

        CGFloat dx = -event.scrollingDeltaX;
        CGFloat dy = -event.scrollingDeltaY;

        // TODO: Allow for easy pure vertical/horizontal scroll like Sublime Text.
        //       Reject slight scrolling deviations in the orthogonal direction.
        // if (abs(dx) <= 1) {
        //     dx = 0;
        // }

        // https://linebender.gitbook.io/linebender-graphics-wiki/mouse-wheel#macos
        if (!event.hasPreciseScrollingDeltas) {
            dx *= 16;
            dy *= 16;
        }

        // Clamps `dx` and `dy` to prevent scrolling beyond buffer boundaries.
        float dx_clamped = std::clamp(openGLLayer->scroll_x + dx, 0.0, [openGLLayer maxScrollX]) -
                           openGLLayer->scroll_x;
        float dy_clamped = std::clamp(openGLLayer->scroll_y + dy, 0.0, [openGLLayer maxScrollY]) -
                           openGLLayer->scroll_y;

        openGLLayer->scroll_x += dx_clamped;
        openGLLayer->scroll_y += dy_clamped;

        // https://developer.apple.com/documentation/appkit/nsevent/1527943-pressedmousebuttons?language=objc
        if (NSEvent.pressedMouseButtons & (1 << 0)) {
            openGLLayer->cursor_end_x += dx_clamped;
            openGLLayer->cursor_end_y += dy_clamped;

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
        openGLLayer.asynchronous = false;  // DEBUG: Remove this.

        for (size_t i = 0; str[i] != '\0'; i++) {
            std::cerr << hex(str[i]) << " ";
        }
        std::cerr << '\n';

        if (str[0] == 0x0D) {
            std::cerr << "new line inserted\n";
            str = "\n";
        }
        // Delete key.
        if (event.keyCode == 117) {
            std::cerr << "delete pressed\n";
            // TODO: Don't hard code to 1 byte removal.
            [openGLLayer removeBytes:1];
            [self.layer setNeedsDisplay];
            return;
        }
        if (str[0] == 0x7F) {
            std::cerr << "backspace pressed\n";
            // TODO: Don't hard code to 1 byte removal.
            [openGLLayer backspaceBytes:1];
            [self.layer setNeedsDisplay];
            return;
        }

        [openGLLayer insertUTF8String:str bytes:bytes];
        [self.layer setNeedsDisplay];
    }
}

- (void)mouseDown:(NSEvent*)event {
    CGFloat mouse_x = event.locationInWindow.x;
    CGFloat mouse_y = event.locationInWindow.y;
    mouse_y = openGLLayer.frame.size.height - mouse_y;  // Set origin at top left.

    if (mouse_x >= openGLLayer->editor_offset_x && mouse_y >= openGLLayer->editor_offset_y) {
        mouse_x -= openGLLayer->editor_offset_x;
        mouse_y -= openGLLayer->editor_offset_y;

        openGLLayer->cursor_start_x = mouse_x + openGLLayer->scroll_x;
        openGLLayer->cursor_start_y = mouse_y + openGLLayer->scroll_y;
        openGLLayer->cursor_end_x = mouse_x + openGLLayer->scroll_x;
        openGLLayer->cursor_end_y = mouse_y + openGLLayer->scroll_y;
        [openGLLayer setRendererCursorPositions];
    } else if (mouse_x < openGLLayer->editor_offset_x) {
        isDragging = true;
    }
}

- (void)mouseUp:(NSEvent*)event {
    isDragging = false;
}

- (void)mouseDragged:(NSEvent*)event {
    CGFloat mouse_x = event.locationInWindow.x;
    CGFloat mouse_y = event.locationInWindow.y;
    mouse_y = openGLLayer.frame.size.height - mouse_y;  // Set origin at top left.

    if (isDragging) {
        openGLLayer->editor_offset_x += event.deltaX;
        [openGLLayer setNeedsDisplay];
    } else {
        mouse_x -= openGLLayer->editor_offset_x;
        mouse_y -= openGLLayer->editor_offset_y;

        openGLLayer->cursor_end_x = mouse_x + openGLLayer->scroll_x;
        openGLLayer->cursor_end_y = mouse_y + openGLLayer->scroll_y;
        [openGLLayer setRendererCursorPositions];
    }
}

- (void)rightMouseDown:(NSEvent*)event {
    NSMenu* contextMenu = [[NSMenu alloc] initWithTitle:@"Contextual Menu"];
    [contextMenu addItemWithTitle:@"Insert test string"
                           action:@selector(insertTestString)
                    keyEquivalent:@""];
    [contextMenu popUpMenuPositioningItem:nil atLocation:event.locationInWindow inView:self];
}

- (void)insertTestString {
    [openGLLayer insertUTF8String:"∆" bytes:3];
    [self.layer setNeedsDisplay];
}

// TODO: Implement light/dark mode detection.
- (void)viewDidChangeEffectiveAppearance {
    // std::cerr << "viewDidChangeEffectiveAppearance\n";
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
        kCGLPFATripleBuffer,
        // kCGLPFADoubleBuffer,
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

- (void)parseBuffer {
    TSInput input = {&buffer, Buffer::read, TSInputEncodingUTF8};
    highlighter.parse(input);
}

- (void)editBuffer:(size_t)bytes {
    size_t start_byte =
        buffer.byteOfLine(text_renderer.cursor_end_line) + text_renderer.cursor_end_col_offset;
    size_t old_end_byte =
        buffer.byteOfLine(text_renderer.cursor_end_line) + text_renderer.cursor_end_col_offset;
    size_t new_end_byte = buffer.byteOfLine(text_renderer.cursor_end_line) +
                          text_renderer.cursor_end_col_offset + bytes;
    highlighter.edit(start_byte, old_end_byte, new_end_byte);
}

- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat {
    CGLContextObj glContext = nullptr;
    CGLCreateContext(pixelFormat, nullptr, &glContext);
    if (glContext || (glContext = [super copyCGLContextForPixelFormat:pixelFormat])) {
        CGLSetCurrentContext(glContext);

        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);

        // if (self.asynchronous) {
        //     glClearColor(240 / 255.0, 240 / 255.0, 240 / 255.0, 1.0);
        // } else {
        //     glClearColor(253 / 255.0, 253 / 255.0, 253 / 255.0, 1.0);
        // }
        glClearColor(253 / 255.0, 253 / 255.0, 253 / 255.0, 1.0);

        int font_size = 16 * self.contentsScale;
        std::string font_name = "Source Code Pro";
        // fs::path file_path = ResourcePath() / "sample_files/text_renderer.cc";
        fs::path file_path = ResourcePath() / "sample_files/example.json";

        float scaled_width = self.frame.size.width * self.contentsScale;
        float scaled_height = self.frame.size.height * self.contentsScale;

        text_renderer.setup(scaled_width, scaled_height, font_name, font_size);
        rect_renderer.setup(scaled_width, scaled_height);
        image_renderer.setup(scaled_width, scaled_height);
        // highlighter.setLanguage("source.c++");
        highlighter.setLanguage("source.json");

        buffer.setContents(ReadFile(file_path));

        // FIXME: Use locks to prevent race conditions.
        // std::thread parse_thread([&] {
        //     {
        //         PROFILE_BLOCK("Tree-sitter only parse");
        //         [self parseBuffer];
        //     }
        // });
        // parse_thread.detach();
        {
            PROFILE_BLOCK("Tree-sitter only parse");
            [self parseBuffer];
        }

        [self addObserver:self forKeyPath:@"bounds" options:0 context:nil];

        editor_offset_x = 200;
        editor_offset_y = 30;

        fs::create_directory(DataPath());
        std::ofstream settings_file(DataPath() / "settings.json");
        if (settings_file.is_open()) {
            settings_file << "test";
            settings_file.flush();
            settings_file.close();
        } else {
            std::cerr << "Error writing to settings.json.\n";
        }
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

    {
        PROFILE_BLOCK("draw");
        // [NSThread sleepForTimeInterval:0.02];  // Simulate lag.

        float scaled_scroll_x = scroll_x * self.contentsScale;
        float scaled_scroll_y = scroll_y * self.contentsScale;
        float scaled_width = self.frame.size.width * self.contentsScale;
        float scaled_height = self.frame.size.height * self.contentsScale;
        float scaled_editor_offset_x = editor_offset_x * self.contentsScale;
        float scaled_editor_offset_y = editor_offset_y * self.contentsScale;

        // if (editor_offset_x > width / 4) {
        //     isSideBarExpanding = false;
        // }
        // if (editor_offset_x < 0) {
        //     isSideBarExpanding = true;
        // }

        // if (isSideBarExpanding) {
        //     editor_offset_x += 1;
        // } else {
        //     editor_offset_x -= 1;
        // }

        glClear(GL_COLOR_BUFFER_BIT);

        glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
        text_renderer.resize(scaled_width, scaled_height);
        text_renderer.renderText(scaled_scroll_x, scaled_scroll_y, buffer, highlighter,
                                 scaled_editor_offset_x, scaled_editor_offset_y);

        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
        rect_renderer.resize(scaled_width, scaled_height);
        rect_renderer.draw(scaled_scroll_x, scaled_scroll_y, text_renderer.cursor_end_x,
                           text_renderer.cursor_end_line, text_renderer.line_height,
                           buffer.lineCount(), text_renderer.longest_line_x,
                           scaled_editor_offset_x, scaled_editor_offset_y);

        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
        image_renderer.resize(scaled_width, scaled_height);
        image_renderer.draw(scaled_scroll_x, scaled_scroll_y, scaled_editor_offset_x,
                            scaled_editor_offset_y);

        // Calls glFlush() by default.
        [super drawInCGLContext:glContext
                    pixelFormat:pixelFormat
                   forLayerTime:timeInterval
                    displayTime:timeStamp];
    }
}

- (void)insertUTF8String:(const char*)str bytes:(size_t)bytes {
    {
        PROFILE_BLOCK("buffer.insert()");
        buffer.insert(text_renderer.cursor_end_line, text_renderer.cursor_end_col_offset, str);
    }

    {
        PROFILE_BLOCK("editBuffer + parseBuffer");
        [self editBuffer:bytes];
        [self parseBuffer];

        // FIXME: This doesn't actually update the logical cursor position!
        if (strcmp(str, "\n") == 0) {
            text_renderer.cursor_start_line++;
            text_renderer.cursor_end_line++;

            text_renderer.cursor_start_col_offset = 0;
            text_renderer.cursor_start_x = 0;
            text_renderer.cursor_end_col_offset = 0;
            text_renderer.cursor_end_x = 0;
        } else {
            float advance = text_renderer.getGlyphAdvance(std::string(str));
            text_renderer.cursor_start_col_offset += bytes;
            text_renderer.cursor_start_x += advance;
            text_renderer.cursor_end_col_offset += bytes;
            text_renderer.cursor_end_x += advance;
        }
    }
}

- (void)removeBytes:(size_t)bytes {
    {
        PROFILE_BLOCK("buffer.remove()");
        buffer.remove(text_renderer.cursor_end_line, text_renderer.cursor_end_col_offset, bytes);
    }

    // // FIXME: Calculate Tree-sitter edits correctly.
    // {
    //     PROFILE_BLOCK("editBuffer + parseBuffer");
    //     // [self editBuffer:bytes];
    //     size_t start_byte =
    //         buffer.byteOfLine(text_renderer.cursor_end_line) +
    //         text_renderer.cursor_end_col_offset;
    //     size_t old_end_byte =
    //         buffer.byteOfLine(text_renderer.cursor_end_line) +
    //         text_renderer.cursor_end_col_offset;
    //     size_t new_end_byte = buffer.byteOfLine(text_renderer.cursor_end_line) +
    //                           text_renderer.cursor_end_col_offset - bytes;
    //     highlighter.edit(start_byte, old_end_byte, new_end_byte);
    //     [self parseBuffer];
    // }
}

- (void)backspaceBytes:(size_t)bytes {
    {
        PROFILE_BLOCK("buffer.backspace()");

        buffer.backspace(text_renderer.cursor_end_line, text_renderer.cursor_end_col_offset,
                         bytes);
        if (text_renderer.cursor_end_col_offset != 0) {
            // FIXME: Don't assume monospace font. Properly calculate advance or reimplement this.
            float advance = text_renderer.getGlyphAdvance("m");
            text_renderer.cursor_start_col_offset -= bytes;
            text_renderer.cursor_start_x -= advance;
            text_renderer.cursor_end_col_offset -= bytes;
            text_renderer.cursor_end_x -= advance;
        } else if (text_renderer.cursor_end_line > 0) {
            // FIXME: Properly move cursor to previous line.
            CGFloat scale = self.contentsScale;
            cursor_start_x = 1000000;
            cursor_end_x = 1000000;
            cursor_start_y -= text_renderer.line_height / scale;
            cursor_end_y -= text_renderer.line_height / scale;

            [self setRendererCursorPositions];
        }
    }

    // // FIXME: Calculate Tree-sitter edits correctly.
    // {
    //     PROFILE_BLOCK("editBuffer + parseBuffer");
    //     // [self editBuffer:bytes];
    //     size_t start_byte =
    //         buffer.byteOfLine(text_renderer.cursor_end_line) +
    //         text_renderer.cursor_end_col_offset;
    //     size_t old_end_byte =
    //         buffer.byteOfLine(text_renderer.cursor_end_line) +
    //         text_renderer.cursor_end_col_offset;
    //     size_t new_end_byte = buffer.byteOfLine(text_renderer.cursor_end_line) +
    //                           text_renderer.cursor_end_col_offset - bytes;
    //     highlighter.edit(start_byte, old_end_byte, new_end_byte);
    //     [self parseBuffer];
    // }
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context {
    scroll_x = std::clamp(scroll_x, 0.0, [self maxScrollX]);
    scroll_y = std::clamp(scroll_y, 0.0, [self maxScrollY]);
    [self setNeedsDisplay];
}

- (void)setRendererCursorPositions {
    CGFloat scale = self.contentsScale;
    text_renderer.setCursorPositions(buffer, cursor_start_x * scale, cursor_start_y * scale,
                                     cursor_end_x * scale, cursor_end_y * scale);
    [self setNeedsDisplay];
}

- (CGFloat)maxScrollX {
    // TODO: Formulate max_cursor_x without the need for division.
    CGFloat max_cursor_x = text_renderer.longest_line_x / self.contentsScale;
    max_cursor_x -= self.frame.size.width - self->editor_offset_x;
    if (max_cursor_x < 0) max_cursor_x = 0;
    return max_cursor_x;
}

- (CGFloat)maxScrollY {
    size_t line_count = buffer.lineCount();
    // line_count -= 1;  // TODO: Merge this with RectRenderer.
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
