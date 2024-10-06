#include "GLView.h"
#include "app/cocoa/GLLayer.h"
#include "util/std_print.h"
#include <iostream>

#import <Carbon/Carbon.h>

namespace {

inline void GetPosition(NSEvent* event,
                        GLLayer* glLayer,
                        int& scaled_mouse_x,
                        int& scaled_mouse_y) {
    int mouse_x = std::round(event.locationInWindow.x);
    int mouse_y = std::round(event.locationInWindow.y);
    mouse_y = glLayer.frame.size.height - mouse_y;  // Set origin at top left.

    int scale = glLayer.contentsScale;
    scaled_mouse_x = mouse_x * scale;
    scaled_mouse_y = mouse_y * scale;
}

constexpr app::Key GetKey(unsigned short vk) {
    constexpr struct {
        unsigned short fVK;
        app::Key fKey;
    } gPair[] = {
        // These constants are located in the <Carbon/Carbon.h> header.
        {kVK_ANSI_A, app::Key::kA},
        {kVK_ANSI_B, app::Key::kB},
        {kVK_ANSI_C, app::Key::kC},
        {kVK_ANSI_D, app::Key::kD},
        {kVK_ANSI_E, app::Key::kE},
        {kVK_ANSI_F, app::Key::kF},
        {kVK_ANSI_G, app::Key::kG},
        {kVK_ANSI_H, app::Key::kH},
        {kVK_ANSI_I, app::Key::kI},
        {kVK_ANSI_J, app::Key::kJ},
        {kVK_ANSI_K, app::Key::kK},
        {kVK_ANSI_L, app::Key::kL},
        {kVK_ANSI_M, app::Key::kM},
        {kVK_ANSI_N, app::Key::kN},
        {kVK_ANSI_O, app::Key::kO},
        {kVK_ANSI_P, app::Key::kP},
        {kVK_ANSI_Q, app::Key::kQ},
        {kVK_ANSI_R, app::Key::kR},
        {kVK_ANSI_S, app::Key::kS},
        {kVK_ANSI_T, app::Key::kT},
        {kVK_ANSI_U, app::Key::kU},
        {kVK_ANSI_V, app::Key::kV},
        {kVK_ANSI_W, app::Key::kW},
        {kVK_ANSI_X, app::Key::kX},
        {kVK_ANSI_Y, app::Key::kY},
        {kVK_ANSI_Z, app::Key::kZ},
        {kVK_ANSI_0, app::Key::k0},
        {kVK_ANSI_1, app::Key::k1},
        {kVK_ANSI_2, app::Key::k2},
        {kVK_ANSI_3, app::Key::k3},
        {kVK_ANSI_4, app::Key::k4},
        {kVK_ANSI_5, app::Key::k5},
        {kVK_ANSI_6, app::Key::k6},
        {kVK_ANSI_7, app::Key::k7},
        {kVK_ANSI_8, app::Key::k8},
        {kVK_ANSI_9, app::Key::k9},
        {kVK_Return, app::Key::kEnter},
        {kVK_Delete, app::Key::kBackspace},
        {kVK_LeftArrow, app::Key::kLeftArrow},
        {kVK_RightArrow, app::Key::kRightArrow},
        {kVK_DownArrow, app::Key::kDownArrow},
        {kVK_UpArrow, app::Key::kUpArrow},
    };

    for (size_t i = 0; i < std::size(gPair); ++i) {
        if (gPair[i].fVK == vk) {
            return gPair[i].fKey;
        }
    }

    return app::Key::kNone;
}

constexpr app::ModifierKey GetModifiers(NSEventModifierFlags flags) {
    app::ModifierKey modifiers = app::ModifierKey::kNone;
    if (flags & NSEventModifierFlagShift) {
        modifiers |= app::ModifierKey::kShift;
    }
    if (flags & NSEventModifierFlagControl) {
        modifiers |= app::ModifierKey::kControl;
    }
    if (flags & NSEventModifierFlagOption) {
        modifiers |= app::ModifierKey::kAlt;
    }
    if (flags & NSEventModifierFlagCommand) {
        modifiers |= app::ModifierKey::kSuper;
    }
    return modifiers;
}

}

@interface GLView () {
@public
    GLLayer* glLayer;
    NSTrackingArea* trackingArea;
}
@end

@implementation GLView

- (instancetype)initWithFrame:(NSRect)frame
                    appWindow:(app::Window*)appWindow
                    displaygl:(app::DisplayGL*)displayGL {
    self = [super initWithFrame:frame];
    if (self) {
        glLayer = [[[GLLayer alloc] initWithDisplayGL:displayGL] autorelease];
        glLayer->appWindow = appWindow;

        // glLayer.needsDisplayOnBoundsChange = true;
        // glLayer.asynchronous = true;
        self.layer = glLayer;

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
    [glLayer setNeedsDisplay];

    // TODO: Investigate how this affects performance. This is needed to trigger redraws in the
    // background, but we do not normally need this. Consider disabling normally.
    // https://stackoverflow.com/a/4740299/14698275
    [CATransaction flush];
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

- (void)mouseMoved:(NSEvent*)event {
    glLayer->appWindow->onMouseMove();
}

- (void)mouseEntered:(NSEvent*)event {
    glLayer->appWindow->onMouseMove();
}

// We need to override `cursorUpdate` to stop the event from being passed up in the chain.
// Without this, our `mouseEntered` NSCursor set will be overridden.
// https://stackoverflow.com/a/20197686
- (void)cursorUpdate:(NSEvent*)event {
    glLayer->appWindow->onMouseMove();
}

- (void)mouseExited:(NSEvent*)event {
    glLayer->appWindow->onMouseExit();
}

- (void)scrollWheel:(NSEvent*)event {
    if (event.type == NSEventTypeScrollWheel) {
        if (event.momentumPhase & NSEventPhaseBegan) {
            // TODO: Implement this as a timer.
            // glLayer.asynchronous = true;
        }
        if (event.momentumPhase & NSEventPhaseEnded) {
            glLayer.asynchronous = false;
        }

        int dx = 0;
        int dy = 0;
        if (event.hasPreciseScrollingDeltas) {
            dx = std::round(-event.scrollingDeltaX);
            dy = std::round(-event.scrollingDeltaY);
        } else {
            // Taken from ///chromium/src/ui/events/cocoa/events_mac.mm.
            // static constexpr double kScrollbarPixelsPerCocoaTick = 40.0;
            // dx = -event.deltaX * kScrollbarPixelsPerCocoaTick;
            // dy = -event.deltaY * kScrollbarPixelsPerCocoaTick;

            // https://linebender.gitbook.io/linebender-graphics-wiki/mouse-wheel#macos
            dx = std::round(-event.scrollingDeltaX) * 16;
            dy = std::round(-event.scrollingDeltaY) * 16;
        }

        // TODO: Allow for easy pure vertical/horizontal scroll like Sublime Text.
        //       Reject slight scrolling deviations in the orthogonal direction.
        // if (abs(dx) <= 1) {
        //     dx = 0;
        // }

        int scale = glLayer.contentsScale;
        int scaled_dx = dx * scale;
        int scaled_dy = dy * scale;

        int scaled_mouse_x, scaled_mouse_y;
        GetPosition(event, glLayer, scaled_mouse_x, scaled_mouse_y);

        glLayer->appWindow->onScroll(scaled_mouse_x, scaled_mouse_y, scaled_dx, scaled_dy);
    }
}

- (void)keyDown:(NSEvent*)event {
    NSTextInputContext* inputContext = [self inputContext];

    bool handled = [inputContext handleEvent:event];
    if (!handled) {
        std::println("keyDown was unhandled.");
    }

    app::Key key = GetKey(event.keyCode);
    app::ModifierKey modifiers = GetModifiers(event.modifierFlags);
    glLayer->appWindow->onKeyDown(key, modifiers);
}

- (void)mouseDown:(NSEvent*)event {
    int scaled_mouse_x, scaled_mouse_y;
    GetPosition(event, glLayer, scaled_mouse_x, scaled_mouse_y);
    app::ModifierKey modifiers = GetModifiers(event.modifierFlags);

    app::ClickType click_type = app::ClickType::kSingleClick;
    if (event.clickCount == 2) {
        click_type = app::ClickType::kDoubleClick;
    } else if (event.clickCount >= 3) {
        click_type = app::ClickType::kTripleClick;
    }

    glLayer->appWindow->onLeftMouseDown(scaled_mouse_x, scaled_mouse_y, modifiers, click_type);
}

- (void)mouseUp:(NSEvent*)event {
    glLayer->appWindow->onLeftMouseUp();
}

- (void)mouseDragged:(NSEvent*)event {
    int scaled_mouse_x, scaled_mouse_y;
    GetPosition(event, glLayer, scaled_mouse_x, scaled_mouse_y);
    app::ModifierKey modifiers = GetModifiers(event.modifierFlags);

    app::ClickType click_type = app::ClickType::kSingleClick;
    if (event.clickCount == 2) {
        click_type = app::ClickType::kDoubleClick;
    } else if (event.clickCount >= 3) {
        click_type = app::ClickType::kTripleClick;
    }

    glLayer->appWindow->onLeftMouseDrag(scaled_mouse_x, scaled_mouse_y, modifiers, click_type);
}

- (void)rightMouseDown:(NSEvent*)event {
    NSMenu* contextMenu = [[NSMenu alloc] initWithTitle:@"Contextual Menu"];
    [contextMenu addItemWithTitle:@"Exit" action:@selector(terminate:) keyEquivalent:@""];
    [contextMenu popUpMenuPositioningItem:nil atLocation:event.locationInWindow inView:self];
}

- (void)viewDidChangeEffectiveAppearance {
    glLayer->appWindow->onDarkModeToggle();
}

// NSTextInputClient protocol implementation.

- (BOOL)hasMarkedText {
    return false;
}

- (NSRange)markedRange {
    return NSMakeRange(NSNotFound, 0);
}

- (NSRange)selectedRange {
    return NSMakeRange(NSNotFound, 0);
}

- (void)setMarkedText:(id)string
        selectedRange:(NSRange)selectedRange
     replacementRange:(NSRange)replacementRange {
}

- (void)unmarkText {
}

- (NSArray*)validAttributesForMarkedText {
    return @[];
}

- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range
                                               actualRange:(NSRangePointer)actualRange {
    return nil;
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {
    BOOL isAttributedString = [string isKindOfClass:[NSAttributedString class]];
    NSString* text = isAttributedString ? [string string] : string;
    glLayer->appWindow->onInsertText(text.UTF8String);
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point {
    return 0;
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
    return NSZeroRect;
}

- (void)doCommandBySelector:(SEL)selector {
    NSString* selector_str = NSStringFromSelector(selector);

    // Remove the trailing colon.
    int selector_len = selector_str.length;
    selector_str = [selector_str substringToIndex:selector_len - 1];

    std::string str = selector_str.UTF8String;
    std::println(str);
    if (str == "moveForward" || str == "moveRight") {
        glLayer->appWindow->onAction(app::Action::kMoveForwardByCharacters);
    }
    if (str == "moveForwardAndModifySelection" || str == "moveRightAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveForwardByCharacters, true);
    }
    if (str == "moveBackward" || str == "moveLeft") {
        glLayer->appWindow->onAction(app::Action::kMoveBackwardByCharacters);
    }
    if (str == "moveBackwardAndModifySelection" || str == "moveLeftAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveBackwardByCharacters, true);
    }
    if (str == "moveDown") {
        glLayer->appWindow->onAction(app::Action::kMoveForwardByLines);
    }
    if (str == "moveDownAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveForwardByLines, true);
    }
    if (str == "moveUp") {
        glLayer->appWindow->onAction(app::Action::kMoveBackwardByLines);
    }
    if (str == "moveUpAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveBackwardByLines, true);
    }
    if (str == "moveWordRight") {
        glLayer->appWindow->onAction(app::Action::kMoveForwardByWords);
    }
    if (str == "moveWordRightAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveForwardByWords, true);
    }
    if (str == "moveWordLeft") {
        glLayer->appWindow->onAction(app::Action::kMoveBackwardByWords);
    }
    if (str == "moveWordLeftAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveBackwardByWords, true);
    }
    if (str == "moveToLeftEndOfLine") {
        glLayer->appWindow->onAction(app::Action::kMoveToBOL);
    }
    if (str == "moveToLeftEndOfLineAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveToBOL, true);
    }
    if (str == "moveToRightEndOfLine") {
        glLayer->appWindow->onAction(app::Action::kMoveToEOL);
    }
    if (str == "moveToRightEndOfLineAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveToEOL, true);
    }
    if (str == "moveToBeginningOfParagraph") {
        glLayer->appWindow->onAction(app::Action::kMoveToHardBOL);
    }
    if (str == "moveToBeginningOfParagraphAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveToHardBOL, true);
    }
    if (str == "moveToEndOfParagraph") {
        glLayer->appWindow->onAction(app::Action::kMoveToHardEOL);
    }
    if (str == "moveToEndOfParagraphAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveToHardEOL, true);
    }
    if (str == "moveToBeginningOfDocument") {
        glLayer->appWindow->onAction(app::Action::kMoveToBOF);
    }
    if (str == "moveToEndOfDocument") {
        glLayer->appWindow->onAction(app::Action::kMoveToEOF);
    }
    if (str == "deleteBackward") {
        glLayer->appWindow->onAction(app::Action::kLeftDelete);
    }
    if (str == "deleteForward") {
        glLayer->appWindow->onAction(app::Action::kRightDelete);
    }
    if (str == "deleteWordBackward") {
        glLayer->appWindow->onAction(app::Action::kDeleteWordBackward);
    }
    if (str == "deleteWordForward") {
        glLayer->appWindow->onAction(app::Action::kDeleteWordForward);
    }
    if (str == "insertNewline") {
        glLayer->appWindow->onAction(app::Action::kInsertNewline);
    }
    if (str == "insertTab") {
        glLayer->appWindow->onAction(app::Action::kInsertTab);
    }
}

@end
