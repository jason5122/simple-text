#include "app/cocoa/gl_layer.h"
#include "app/types.h"
#include "gl_view.h"

#import <Carbon/Carbon.h>

// Debug use; remove this.
#include "util/std_print.h"

namespace app {
namespace {

constexpr Key KeyFromKeyCode(unsigned short vk);
constexpr ModifierKey ModifierFromFlags(NSEventModifierFlags flags);
inline Point MousePositionFromEvent(NSEvent* event);
inline Point ScaleAndInvertPosition(const Point& point, GLLayer* glLayer);

}  // namespace
}  // namespace app

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
        int dx = std::round(-event.scrollingDeltaX);
        int dy = std::round(-event.scrollingDeltaY);
        app::Delta scroll{dx, dy};
        if (!event.hasPreciseScrollingDeltas) {
            // Taken from ///chromium/src/ui/events/cocoa/events_mac.mm.
            // static constexpr double kScrollbarPixelsPerCocoaTick = 40.0;
            // int dx = -event.deltaX * kScrollbarPixelsPerCocoaTick;
            // int dy = -event.deltaY * kScrollbarPixelsPerCocoaTick;

            // https://linebender.gitbook.io/linebender-graphics-wiki/mouse-wheel#macos
            scroll *= 16;
        }

        int scale = glLayer.contentsScale;
        scroll *= scale;

        auto mouse_pos = app::ScaleAndInvertPosition(app::MousePositionFromEvent(event), glLayer);

        glLayer->appWindow->onScroll(mouse_pos.x, mouse_pos.y, scroll.dx, scroll.dy);
    }
}

- (void)keyDown:(NSEvent*)event {
    app::Key key = app::KeyFromKeyCode(event.keyCode);
    app::ModifierKey modifiers = app::ModifierFromFlags(event.modifierFlags);
    bool handled = glLayer->appWindow->onKeyDown(key, modifiers);

    if (!handled) {
        // TODO: Should we ignore the return value of this?
        [self.inputContext handleEvent:event];
    }
}

- (void)mouseDown:(NSEvent*)event {
    auto mouse_pos = app::ScaleAndInvertPosition(app::MousePositionFromEvent(event), glLayer);
    app::ModifierKey modifiers = app::ModifierFromFlags(event.modifierFlags);
    app::ClickType click_type = app::ClickTypeFromCount(event.clickCount);
    glLayer->appWindow->onLeftMouseDown(mouse_pos.x, mouse_pos.y, modifiers, click_type);
}

- (void)mouseUp:(NSEvent*)event {
    glLayer->appWindow->onLeftMouseUp();
}

- (void)mouseDragged:(NSEvent*)event {
    auto mouse_pos = app::ScaleAndInvertPosition(app::MousePositionFromEvent(event), glLayer);
    app::ModifierKey modifiers = app::ModifierFromFlags(event.modifierFlags);
    app::ClickType click_type = app::ClickTypeFromCount(event.clickCount);
    glLayer->appWindow->onLeftMouseDrag(mouse_pos.x, mouse_pos.y, modifiers, click_type);
}

- (void)rightMouseDown:(NSEvent*)event {
    auto mouse_pos = app::ScaleAndInvertPosition(app::MousePositionFromEvent(event), glLayer);
    app::ModifierKey modifiers = app::ModifierFromFlags(event.modifierFlags);
    app::ClickType click_type = app::ClickTypeFromCount(event.clickCount);
    glLayer->appWindow->onRightMouseDown(mouse_pos.x, mouse_pos.y, modifiers, click_type);
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
    // return NSZeroRect;

    // TODO: This determines the coordinates of where the IME box appears. Implement this.
    return {
        .origin = {500, 500},
        .size = {0, 0},
    };
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
    } else if (str == "moveForwardAndModifySelection" || str == "moveRightAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveForwardByCharacters, true);
    } else if (str == "moveBackward" || str == "moveLeft") {
        glLayer->appWindow->onAction(app::Action::kMoveBackwardByCharacters);
    } else if (str == "moveBackwardAndModifySelection" || str == "moveLeftAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveBackwardByCharacters, true);
    } else if (str == "moveDown") {
        glLayer->appWindow->onAction(app::Action::kMoveForwardByLines);
    } else if (str == "moveDownAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveForwardByLines, true);
    } else if (str == "moveUp") {
        glLayer->appWindow->onAction(app::Action::kMoveBackwardByLines);
    } else if (str == "moveUpAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveBackwardByLines, true);
    } else if (str == "moveWordRight") {
        glLayer->appWindow->onAction(app::Action::kMoveForwardByWords);
    } else if (str == "moveWordRightAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveForwardByWords, true);
    } else if (str == "moveWordLeft") {
        glLayer->appWindow->onAction(app::Action::kMoveBackwardByWords);
    } else if (str == "moveWordLeftAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveBackwardByWords, true);
    } else if (str == "moveToLeftEndOfLine") {
        glLayer->appWindow->onAction(app::Action::kMoveToBOL);
    } else if (str == "moveToLeftEndOfLineAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveToBOL, true);
    } else if (str == "moveToRightEndOfLine") {
        glLayer->appWindow->onAction(app::Action::kMoveToEOL);
    } else if (str == "moveToRightEndOfLineAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveToEOL, true);
    } else if (str == "moveToBeginningOfParagraph") {
        glLayer->appWindow->onAction(app::Action::kMoveToHardBOL);
    } else if (str == "moveToBeginningOfParagraphAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveToHardBOL, true);
    } else if (str == "moveToEndOfParagraph") {
        glLayer->appWindow->onAction(app::Action::kMoveToHardEOL);
    } else if (str == "moveToEndOfParagraphAndModifySelection") {
        glLayer->appWindow->onAction(app::Action::kMoveToHardEOL, true);
    } else if (str == "moveToBeginningOfDocument") {
        glLayer->appWindow->onAction(app::Action::kMoveToBOF);
    } else if (str == "moveToEndOfDocument") {
        glLayer->appWindow->onAction(app::Action::kMoveToEOF);
    } else if (str == "deleteBackward") {
        glLayer->appWindow->onAction(app::Action::kLeftDelete);
    } else if (str == "deleteForward") {
        glLayer->appWindow->onAction(app::Action::kRightDelete);
    } else if (str == "deleteWordBackward") {
        glLayer->appWindow->onAction(app::Action::kDeleteWordBackward);
    } else if (str == "deleteWordForward") {
        glLayer->appWindow->onAction(app::Action::kDeleteWordForward);
    } else if (str == "insertNewline") {
        glLayer->appWindow->onAction(app::Action::kInsertNewline);
    } else if (str == "insertTab") {
        glLayer->appWindow->onAction(app::Action::kInsertTab);
    }
}

@end

namespace app {
namespace {

constexpr Key KeyFromKeyCode(unsigned short vk) {
    constexpr struct {
        unsigned short fVK;
        Key fKey;
    } gPair[] = {
        // These constants are located in the <Carbon/Carbon.h> header.
        {kVK_ANSI_A, Key::kA},
        {kVK_ANSI_B, Key::kB},
        {kVK_ANSI_C, Key::kC},
        {kVK_ANSI_D, Key::kD},
        {kVK_ANSI_E, Key::kE},
        {kVK_ANSI_F, Key::kF},
        {kVK_ANSI_G, Key::kG},
        {kVK_ANSI_H, Key::kH},
        {kVK_ANSI_I, Key::kI},
        {kVK_ANSI_J, Key::kJ},
        {kVK_ANSI_K, Key::kK},
        {kVK_ANSI_L, Key::kL},
        {kVK_ANSI_M, Key::kM},
        {kVK_ANSI_N, Key::kN},
        {kVK_ANSI_O, Key::kO},
        {kVK_ANSI_P, Key::kP},
        {kVK_ANSI_Q, Key::kQ},
        {kVK_ANSI_R, Key::kR},
        {kVK_ANSI_S, Key::kS},
        {kVK_ANSI_T, Key::kT},
        {kVK_ANSI_U, Key::kU},
        {kVK_ANSI_V, Key::kV},
        {kVK_ANSI_W, Key::kW},
        {kVK_ANSI_X, Key::kX},
        {kVK_ANSI_Y, Key::kY},
        {kVK_ANSI_Z, Key::kZ},
        {kVK_ANSI_0, Key::k0},
        {kVK_ANSI_1, Key::k1},
        {kVK_ANSI_2, Key::k2},
        {kVK_ANSI_3, Key::k3},
        {kVK_ANSI_4, Key::k4},
        {kVK_ANSI_5, Key::k5},
        {kVK_ANSI_6, Key::k6},
        {kVK_ANSI_7, Key::k7},
        {kVK_ANSI_8, Key::k8},
        {kVK_ANSI_9, Key::k9},
        {kVK_Return, Key::kEnter},
        {kVK_Delete, Key::kBackspace},
        {kVK_Tab, Key::kTab},
        {kVK_LeftArrow, Key::kLeftArrow},
        {kVK_RightArrow, Key::kRightArrow},
        {kVK_DownArrow, Key::kDownArrow},
        {kVK_UpArrow, Key::kUpArrow},
    };

    for (size_t i = 0; i < std::size(gPair); ++i) {
        if (gPair[i].fVK == vk) {
            return gPair[i].fKey;
        }
    }

    return Key::kNone;
}

constexpr ModifierKey ModifierFromFlags(NSEventModifierFlags flags) {
    ModifierKey modifiers = ModifierKey::kNone;
    if (flags & NSEventModifierFlagShift) {
        modifiers |= ModifierKey::kShift;
    }
    if (flags & NSEventModifierFlagControl) {
        modifiers |= ModifierKey::kControl;
    }
    if (flags & NSEventModifierFlagOption) {
        modifiers |= ModifierKey::kAlt;
    }
    if (flags & NSEventModifierFlagCommand) {
        modifiers |= ModifierKey::kSuper;
    }
    return modifiers;
}

inline Point MousePositionFromEvent(NSEvent* event) {
    int mouse_x = std::round(event.locationInWindow.x);
    int mouse_y = std::round(event.locationInWindow.y);
    return {mouse_x, mouse_y};
}

inline Point ScaleAndInvertPosition(const Point& point, GLLayer* glLayer) {
    int scale = glLayer.contentsScale;
    int window_height = glLayer.frame.size.height;

    Point new_point = point;
    new_point.y = window_height - new_point.y;  // Set origin at top left.
    new_point *= scale;
    return new_point;
}

}  // namespace
}  // namespace app
