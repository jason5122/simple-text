#include "OpenGLView.h"
#include <glad/glad.h>
#include <iostream>

#import <Carbon/Carbon.h>

@interface OpenGLLayer : CAOpenGLLayer {
@public
    App::Window* appWindow;

@private
    CGLContextObj mContext;
    DisplayGL* displaygl;
}

- (instancetype)initWithDisplayGL:(DisplayGL*)theDisplaygl;
@end

@interface OpenGLView () {
@public
    OpenGLLayer* openGLLayer;
    NSTrackingArea* trackingArea;
}
@end

@implementation OpenGLView

- (instancetype)initWithFrame:(NSRect)frame
                    appWindow:(App::Window*)theAppWindow
                    displaygl:(DisplayGL*)displaygl {
    self = [super initWithFrame:frame];
    if (self) {
        openGLLayer = [[[OpenGLLayer alloc] initWithDisplayGL:displaygl] autorelease];
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
        trackingArea = [[[NSTrackingArea alloc] initWithRect:self.bounds
                                                     options:options
                                                       owner:self
                                                    userInfo:nil] autorelease];
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
    trackingArea = [[[NSTrackingArea alloc] initWithRect:self.bounds
                                                 options:options
                                                   owner:self
                                                userInfo:nil] autorelease];
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
            openGLLayer.asynchronous = true;
        }
        if (event.momentumPhase & NSEventPhaseEnded) {
            openGLLayer.asynchronous = false;
        }

        CGFloat dx = 0;
        CGFloat dy = 0;
        if (event.hasPreciseScrollingDeltas) {
            dx = -event.scrollingDeltaX;
            dy = -event.scrollingDeltaY;
        } else {
            // Taken from ///chromium/src/ui/events/cocoa/events_mac.mm.
            // static constexpr double kScrollbarPixelsPerCocoaTick = 40.0;
            // dx = -event.deltaX * kScrollbarPixelsPerCocoaTick;
            // dy = -event.deltaY * kScrollbarPixelsPerCocoaTick;

            // https://linebender.gitbook.io/linebender-graphics-wiki/mouse-wheel#macos
            dx = -event.scrollingDeltaX * 16;
            dy = -event.scrollingDeltaY * 16;
        }

        // TODO: Allow for easy pure vertical/horizontal scroll like Sublime Text.
        //       Reject slight scrolling deviations in the orthogonal direction.
        // if (abs(dx) <= 1) {
        //     dx = 0;
        // }

        float scaled_dx = dx * openGLLayer.contentsScale;
        float scaled_dy = dy * openGLLayer.contentsScale;
        openGLLayer->appWindow->onScroll(scaled_dx, scaled_dy);
    }
}

static app::Key GetKey(unsigned short vk) {
    static constexpr struct {
        unsigned short fVK;
        app::Key fKey;
    } gPair[] = {
        // These constants are located in the <Carbon/Carbon.h> header.
        {kVK_ANSI_A, app::Key::kA}, {kVK_ANSI_B, app::Key::kB}, {kVK_ANSI_C, app::Key::kC},
        {kVK_ANSI_D, app::Key::kD}, {kVK_ANSI_E, app::Key::kE}, {kVK_ANSI_F, app::Key::kF},
        {kVK_ANSI_G, app::Key::kG}, {kVK_ANSI_H, app::Key::kH}, {kVK_ANSI_I, app::Key::kI},
        {kVK_ANSI_J, app::Key::kJ}, {kVK_ANSI_K, app::Key::kK}, {kVK_ANSI_L, app::Key::kL},
        {kVK_ANSI_M, app::Key::kM}, {kVK_ANSI_N, app::Key::kN}, {kVK_ANSI_O, app::Key::kO},
        {kVK_ANSI_P, app::Key::kP}, {kVK_ANSI_Q, app::Key::kQ}, {kVK_ANSI_R, app::Key::kR},
        {kVK_ANSI_S, app::Key::kS}, {kVK_ANSI_T, app::Key::kT}, {kVK_ANSI_U, app::Key::kU},
        {kVK_ANSI_V, app::Key::kV}, {kVK_ANSI_W, app::Key::kW}, {kVK_ANSI_X, app::Key::kX},
        {kVK_ANSI_Y, app::Key::kY}, {kVK_ANSI_Z, app::Key::kZ}, {kVK_ANSI_0, app::Key::k0},
        {kVK_ANSI_1, app::Key::k1}, {kVK_ANSI_2, app::Key::k2}, {kVK_ANSI_3, app::Key::k3},
        {kVK_ANSI_4, app::Key::k4}, {kVK_ANSI_5, app::Key::k5}, {kVK_ANSI_6, app::Key::k6},
        {kVK_ANSI_7, app::Key::k7}, {kVK_ANSI_8, app::Key::k8}, {kVK_ANSI_9, app::Key::k9},
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
    openGLLayer->appWindow->onLeftMouseDown(scaled_mouse_x, scaled_mouse_y);
}

- (void)mouseDragged:(NSEvent*)event {
    CGFloat mouse_x = event.locationInWindow.x;
    CGFloat mouse_y = event.locationInWindow.y;
    mouse_y = openGLLayer.frame.size.height - mouse_y;  // Set origin at top left.

    float scaled_mouse_x = mouse_x * openGLLayer.contentsScale;
    float scaled_mouse_y = mouse_y * openGLLayer.contentsScale;
    openGLLayer->appWindow->onLeftMouseDrag(scaled_mouse_x, scaled_mouse_y);
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

- (instancetype)initWithDisplayGL:(DisplayGL*)theDisplaygl {
    self = [super init];
    if (self) {
        displaygl = theDisplaygl;
    }
    return self;
}

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
    return displaygl->mPixelFormat;
}

- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat {
    CGLSetCurrentContext(displaygl->mContext);

    int scaled_width = self.frame.size.width * self.contentsScale;
    int scaled_height = self.frame.size.height * self.contentsScale;

    appWindow->onOpenGLActivate(scaled_width, scaled_height);

    [self addObserver:self forKeyPath:@"bounds" options:0 context:nil];

    return displaygl->mContext;
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
    CGLSetCurrentContext(displaygl->mContext);

    appWindow->onDraw();

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
    CGLSetCurrentContext(displaygl->mContext);

    float scaled_width = self.frame.size.width * self.contentsScale;
    float scaled_height = self.frame.size.height * self.contentsScale;
    appWindow->onResize(scaled_width, scaled_height);
}

// We shouldn't release the CGLContextObj since it isn't owned by this object.
- (void)releaseCGLContext:(CGLContextObj)glContext {
}

// We shouldn't release the CGLPixelFormatObj since it isn't owned by this object.
- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pixelFormat {
}

@end
