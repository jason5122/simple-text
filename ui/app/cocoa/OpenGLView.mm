#include "OpenGLView.h"
#include <glad/glad.h>
#include <iostream>

#import <Carbon/Carbon.h>

@interface OpenGLLayer : CAOpenGLLayer {
@public
    App::Window* appWindow;

@private
    CGLContextObj mContext;
}
@end

@interface OpenGLView () {
@public
    OpenGLLayer* openGLLayer;
    NSTrackingArea* trackingArea;
}
@end

@implementation OpenGLView

- (instancetype)initWithFrame:(NSRect)frame appWindow:(App::Window*)theAppWindow {
    self = [super initWithFrame:frame];
    if (self) {
        openGLLayer = [OpenGLLayer layer];
        openGLLayer->appWindow = theAppWindow;

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
    }
    return self;
}

- (void)redraw {
    [openGLLayer setNeedsDisplay];
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

    [NSCursor.IBeamCursor set];
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
            // openGLLayer.asynchronous = true;
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

        float scaled_dx = dx * openGLLayer.contentsScale;
        float scaled_dy = dy * openGLLayer.contentsScale;
        // openGLLayer->appWindow->onScroll(scaled_dx, scaled_dy);
    }
}

static app::Key GetKey(unsigned short vk) {
    static const struct {
        unsigned short fVK;
        app::Key fKey;
    } gPair[] = {
        // These constants are located in the <Carbon/Carbon.h> header.
        {kVK_ANSI_A, app::Key::kA},
        {kVK_ANSI_B, app::Key::kB},
        {kVK_ANSI_C, app::Key::kC},
        // TODO: Implement the rest.
        {kVK_ANSI_N, app::Key::kN},
        {kVK_ANSI_Q, app::Key::kQ},
        {kVK_ANSI_W, app::Key::kW},
    };

    for (size_t i = 0; i < std::size(gPair); i++) {
        if (gPair[i].fVK == vk) {
            return gPair[i].fKey;
        }
    }

    return app::Key::kNone;
}

- (void)keyDown:(NSEvent*)event {
    app::Key key = GetKey(event.keyCode);

    app::ModifierKey modifiers = app::ModifierKey::kNone;
    if (event.modifierFlags & NSEventModifierFlagShift) {
        modifiers |= app::ModifierKey::kShift;
    }
    if (event.modifierFlags & NSEventModifierFlagControl) {
        modifiers |= app::ModifierKey::kControl;
    }
    if (event.modifierFlags & NSEventModifierFlagOption) {
        modifiers |= app::ModifierKey::kAlt;
    }
    if (event.modifierFlags & NSEventModifierFlagCommand) {
        modifiers |= app::ModifierKey::kSuper;
    }

    openGLLayer->appWindow->onKeyDown(key, modifiers);
}

- (void)mouseDown:(NSEvent*)event {
    CGFloat mouse_x = event.locationInWindow.x;
    CGFloat mouse_y = event.locationInWindow.y;
    mouse_y = openGLLayer.frame.size.height - mouse_y;  // Set origin at top left.

    float scaled_mouse_x = mouse_x * openGLLayer.contentsScale;
    float scaled_mouse_y = mouse_y * openGLLayer.contentsScale;
    // openGLLayer->appWindow->onLeftMouseDown(scaled_mouse_x, scaled_mouse_y);
}

- (void)mouseDragged:(NSEvent*)event {
    CGFloat mouse_x = event.locationInWindow.x;
    CGFloat mouse_y = event.locationInWindow.y;
    mouse_y = openGLLayer.frame.size.height - mouse_y;  // Set origin at top left.

    float scaled_mouse_x = mouse_x * openGLLayer.contentsScale;
    float scaled_mouse_y = mouse_y * openGLLayer.contentsScale;
    // openGLLayer->appWindow->onLeftMouseDrag(scaled_mouse_x, scaled_mouse_y);
}

- (void)rightMouseDown:(NSEvent*)event {
    NSMenu* contextMenu = [[NSMenu alloc] initWithTitle:@"Contextual Menu"];
    [contextMenu addItemWithTitle:@"Insert test string"
                           action:@selector(insertTestString)
                    keyEquivalent:@""];
    [contextMenu popUpMenuPositioningItem:nil atLocation:event.locationInWindow inView:self];
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

- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat {
    CGLCreateContext(pixelFormat, nullptr, &mContext);

    if (mContext || (mContext = [super copyCGLContextForPixelFormat:pixelFormat])) {
        CGLSetCurrentContext(mContext);

        if (!gladLoadGL()) {
            std::cerr << "Failed to initialize GLAD\n";
        }

        int scaled_width = self.frame.size.width * self.contentsScale;
        int scaled_height = self.frame.size.height * self.contentsScale;

        // appWindow->onOpenGLActivate(scaled_width, scaled_height);

        [self addObserver:self forKeyPath:@"bounds" options:0 context:nil];
    }
    return mContext;
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
    CGLSetCurrentContext(mContext);

    // appWindow->onDraw();

    // Calls glFlush() by default.
    [super drawInCGLContext:mContext
                pixelFormat:pixelFormat
               forLayerTime:timeInterval
                displayTime:timeStamp];
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context {
    CGLSetCurrentContext(mContext);

    float scaled_width = self.frame.size.width * self.contentsScale;
    float scaled_height = self.frame.size.height * self.contentsScale;
    // appWindow->onResize(scaled_width, scaled_height);
}

- (void)releaseCGLContext:(CGLContextObj)glContext {
    [super releaseCGLContext:glContext];
}

- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pixelFormat {
    [super releaseCGLPixelFormat:pixelFormat];
}

@end
