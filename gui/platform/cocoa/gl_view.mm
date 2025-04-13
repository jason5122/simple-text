#include "gl_view.h"

#include "gui/platform/cocoa/gl_layer.h"
#include "gui/types.h"

#include <Carbon/Carbon.h>

// Debug use; remove this.
#include <fmt/base.h>

namespace gui {
namespace {

constexpr Key KeyFromKeyCode(unsigned short vk);
constexpr ModifierKey ModifierFromFlags(NSEventModifierFlags flags);
inline Point MousePositionFromEvent(NSEvent* event);
inline Point ScaleAndInvertPosition(const Point& point, GLLayer* gl_layer);

}  // namespace
}  // namespace gui

@interface GLView () {
@public
    GLLayer* gl_layer;
    NSTrackingArea* trackingArea;
    gui::WindowWidget* app_window;
}

- (void)onScrollerStyleChanged:(NSNotification*)notification;

@end

@implementation GLView

- (instancetype)initWithFrame:(NSRect)frame
                    appWindow:(gui::WindowWidget*)appWindow
                    displayGL:(gui::DisplayGL*)displayGL {
    self = [super initWithFrame:frame];
    if (self) {
        gl_layer = [[[GLLayer alloc] initWithAppWindow:appWindow displayGL:displayGL] autorelease];
        app_window = appWindow;

        gl_layer.needsDisplayOnBoundsChange = true;
        // gl_layer.asynchronous = true;
        self.layer = gl_layer;

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

        // Listen for changes to "Show scroll bars" setting.
        [NSNotificationCenter.defaultCenter
            addObserver:self
               selector:@selector(onScrollerStyleChanged:)
                   name:NSPreferredScrollerStyleDidChangeNotification
                 object:nil];
        // Seems that we need to call NSScroller.preferredScrollerStyle once to listen to updates.
        [NSScroller preferredScrollerStyle];

        // Listen for changes to "Click in the scroll bar to..." setting.
        NSUserDefaults* defaults = NSUserDefaults.standardUserDefaults;
        [defaults addObserver:self
                   forKeyPath:@"AppleScrollerPagingBehavior"
                      options:NSKeyValueObservingOptionNew
                      context:nullptr];
    }
    return self;
}

- (void)redraw {
    [gl_layer setNeedsDisplay];

    // TODO: Investigate how this affects performance. This is needed to trigger redraws in the
    // background, but we do not normally need this. Consider disabling normally.
    // https://stackoverflow.com/a/4740299/14698275
    // [CATransaction flush];
}

- (void)invalidateAppWindowPointer {
    NSUserDefaults* defaults = NSUserDefaults.standardUserDefaults;
    [defaults removeObserver:self forKeyPath:@"AppleScrollerPagingBehavior" context:nullptr];

    app_window = nullptr;
    [gl_layer invalidateAppWindowPointer];
}

- (void)setAutoRedraw:(bool)autoRedraw {
    [gl_layer setAutoRedraw:autoRedraw];
}

- (int)framesPerSecond {
    return [gl_layer framesPerSecond];
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
    if (app_window) {
        auto mouse_pos = gui::ScaleAndInvertPosition(gui::MousePositionFromEvent(event), gl_layer);
        app_window->mouse_position_changed(mouse_pos);
    }
}

- (void)mouseEntered:(NSEvent*)event {
    if (app_window) {
        auto mouse_pos = gui::ScaleAndInvertPosition(gui::MousePositionFromEvent(event), gl_layer);
        app_window->mouse_position_changed(mouse_pos);
    }
}

// We need to override `cursorUpdate` to stop the event from being passed up in the chain.
// Without this, our `mouseEntered` NSCursor set will be overridden.
// https://stackoverflow.com/a/20197686
- (void)cursorUpdate:(NSEvent*)event {
    if (app_window) {
        auto mouse_pos = gui::ScaleAndInvertPosition(gui::MousePositionFromEvent(event), gl_layer);
        app_window->mouse_position_changed(mouse_pos);
    }
}

- (void)mouseExited:(NSEvent*)event {
    if (app_window) {
        app_window->mouse_position_changed(std::nullopt);
    }
}

- (void)scrollWheel:(NSEvent*)event {
    if (!app_window) return;

    if (event.type == NSEventTypeScrollWheel) {
        double raw_x = -event.scrollingDeltaX;
        double raw_y = -event.scrollingDeltaY;
        if (!event.hasPreciseScrollingDeltas) {
            // TODO: Do we use this code instead?
            // Taken from //chromium/src/ui/events/cocoa/events_mac.mm.
            // static constexpr double kScrollbarPixelsPerCocoaTick = 40.0;
            // int dx = -event.deltaX * kScrollbarPixelsPerCocoaTick;
            // int dy = -event.deltaY * kScrollbarPixelsPerCocoaTick;

            // https://linebender.gitbook.io/linebender-graphics-wiki/mouse-wheel#macos
            raw_x *= 16;
            raw_y *= 16;
        }

        int dx = std::ceil(raw_x);
        int dy = std::ceil(raw_y);
        gui::Delta scroll = {dx, dy};

        int scale = gl_layer.contentsScale;
        scroll *= scale;

        auto mouse_pos = gui::ScaleAndInvertPosition(gui::MousePositionFromEvent(event), gl_layer);
        app_window->perform_scroll(mouse_pos, scroll);
    }
}

- (void)keyDown:(NSEvent*)event {
    if (app_window) {
        gui::Key key = gui::KeyFromKeyCode(event.keyCode);
        gui::ModifierKey modifiers = gui::ModifierFromFlags(event.modifierFlags);
        bool handled = app_window->on_key_down(key, modifiers);

        if (!handled) {
            // TODO: Should we ignore the return value of this?
            [self.inputContext handleEvent:event];
        }
    }
}

- (void)mouseDown:(NSEvent*)event {
    if (app_window) {
        auto mouse_pos = gui::ScaleAndInvertPosition(gui::MousePositionFromEvent(event), gl_layer);
        gui::ModifierKey modifiers = gui::ModifierFromFlags(event.modifierFlags);
        gui::ClickType click_type = gui::click_type_from_count(event.clickCount);
        app_window->left_mouse_down(mouse_pos, modifiers, click_type);
    }
}

- (void)mouseUp:(NSEvent*)event {
    if (app_window) {
        auto mouse_pos = gui::ScaleAndInvertPosition(gui::MousePositionFromEvent(event), gl_layer);
        app_window->left_mouse_up(mouse_pos);
    }
}

- (void)mouseDragged:(NSEvent*)event {
    if (app_window) {
        auto mouse_pos = gui::ScaleAndInvertPosition(gui::MousePositionFromEvent(event), gl_layer);
        gui::ModifierKey modifiers = gui::ModifierFromFlags(event.modifierFlags);
        gui::ClickType click_type = gui::click_type_from_count(event.clickCount);
        app_window->left_mouse_drag(mouse_pos, modifiers, click_type);
    }
}

- (void)rightMouseDown:(NSEvent*)event {
    if (app_window) {
        auto mouse_pos = gui::ScaleAndInvertPosition(gui::MousePositionFromEvent(event), gl_layer);
        gui::ModifierKey modifiers = gui::ModifierFromFlags(event.modifierFlags);
        gui::ClickType click_type = gui::click_type_from_count(event.clickCount);
        app_window->right_mouse_down(mouse_pos, modifiers, click_type);
    }
}

- (void)viewDidChangeEffectiveAppearance {
    if (app_window) {
        app_window->on_dark_mode_toggle();
    }
}

// Listen for changes to "Show scroll bars" setting.
- (void)onScrollerStyleChanged:(NSNotification*)notification {
    auto style = NSScroller.preferredScrollerStyle;
    if (style == NSScrollerStyleLegacy) {
        fmt::println("NSScroller.preferredScrollerStyle is now NSScrollerStyleLegacy.");
    } else if (style == NSScrollerStyleOverlay) {
        fmt::println("NSScroller.preferredScrollerStyle is now NSScrollerStyleOverlay.");
    }
}

// Listen for changes to "Click in the scroll bar to..." setting.
- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context {
    NSUserDefaults* defaults = NSUserDefaults.standardUserDefaults;
    bool jump_on_scroll_bar_click = [defaults boolForKey:@"AppleScrollerPagingBehavior"];
    fmt::println("jump on scroll bar click (update): {}", jump_on_scroll_bar_click);
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
    if (app_window) {
        BOOL isAttributedString = [string isKindOfClass:NSAttributedString.class];
        NSString* text = isAttributedString ? [string string] : string;
        app_window->on_insert_text(text.UTF8String);
    }
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
    if (!app_window) return;

    NSString* selector_str = NSStringFromSelector(selector);

    // Remove the trailing colon.
    auto len = selector_str.length;
    if (len > 0) {
        selector_str = [selector_str substringToIndex:len - 1];
    }

    std::string str = selector_str.UTF8String;
    if (str == "moveForward" || str == "moveRight") {
        app_window->on_action(gui::Action::kMoveForwardByCharacters);
    } else if (str == "moveForwardAndModifySelection" || str == "moveRightAndModifySelection") {
        app_window->on_action(gui::Action::kMoveForwardByCharacters, true);
    } else if (str == "moveBackward" || str == "moveLeft") {
        app_window->on_action(gui::Action::kMoveBackwardByCharacters);
    } else if (str == "moveBackwardAndModifySelection" || str == "moveLeftAndModifySelection") {
        app_window->on_action(gui::Action::kMoveBackwardByCharacters, true);
    } else if (str == "moveDown") {
        app_window->on_action(gui::Action::kMoveForwardByLines);
    } else if (str == "moveDownAndModifySelection") {
        app_window->on_action(gui::Action::kMoveForwardByLines, true);
    } else if (str == "moveUp") {
        app_window->on_action(gui::Action::kMoveBackwardByLines);
    } else if (str == "moveUpAndModifySelection") {
        app_window->on_action(gui::Action::kMoveBackwardByLines, true);
    } else if (str == "moveWordRight") {
        app_window->on_action(gui::Action::kMoveForwardByWords);
    } else if (str == "moveWordRightAndModifySelection") {
        app_window->on_action(gui::Action::kMoveForwardByWords, true);
    } else if (str == "moveWordLeft") {
        app_window->on_action(gui::Action::kMoveBackwardByWords);
    } else if (str == "moveWordLeftAndModifySelection") {
        app_window->on_action(gui::Action::kMoveBackwardByWords, true);
    } else if (str == "moveToLeftEndOfLine") {
        app_window->on_action(gui::Action::kMoveToBOL);
    } else if (str == "moveToLeftEndOfLineAndModifySelection") {
        app_window->on_action(gui::Action::kMoveToBOL, true);
    } else if (str == "moveToRightEndOfLine") {
        app_window->on_action(gui::Action::kMoveToEOL);
    } else if (str == "moveToRightEndOfLineAndModifySelection") {
        app_window->on_action(gui::Action::kMoveToEOL, true);
    } else if (str == "moveToBeginningOfParagraph") {
        app_window->on_action(gui::Action::kMoveToHardBOL);
    } else if (str == "moveToBeginningOfParagraphAndModifySelection") {
        app_window->on_action(gui::Action::kMoveToHardBOL, true);
    } else if (str == "moveToEndOfParagraph") {
        app_window->on_action(gui::Action::kMoveToHardEOL);
    } else if (str == "moveToEndOfParagraphAndModifySelection") {
        app_window->on_action(gui::Action::kMoveToHardEOL, true);
    } else if (str == "moveToBeginningOfDocument") {
        app_window->on_action(gui::Action::kMoveToBOF);
    } else if (str == "moveToEndOfDocument") {
        app_window->on_action(gui::Action::kMoveToEOF);
    } else if (str == "deleteBackward") {
        app_window->on_action(gui::Action::kLeftDelete);
    } else if (str == "deleteForward") {
        app_window->on_action(gui::Action::kRightDelete);
    } else if (str == "deleteWordBackward") {
        app_window->on_action(gui::Action::kDeleteWordBackward);
    } else if (str == "deleteWordForward") {
        app_window->on_action(gui::Action::kDeleteWordForward);
    } else if (str == "insertNewline") {
        app_window->on_action(gui::Action::kInsertNewline);
    } else if (str == "insertNewlineIgnoringFieldEditor") {
        app_window->on_action(gui::Action::kInsertNewlineIgnoringFieldEditor);
    } else if (str == "insertTab" || str == "insertTabIgnoringFieldEditor") {
        app_window->on_action(gui::Action::kInsertTab);
    }
}

@end

namespace gui {
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
        {kVK_ANSI_Equal, Key::kEqual},
        {kVK_ANSI_Minus, Key::kMinus},
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

inline Point ScaleAndInvertPosition(const Point& point, GLLayer* gl_layer) {
    int scale = gl_layer.contentsScale;
    int window_height = gl_layer.frame.size.height;

    Point new_point = point;
    new_point.y = window_height - new_point.y;  // Set origin at top left.
    new_point *= scale;
    return new_point;
}

}  // namespace
}  // namespace gui
