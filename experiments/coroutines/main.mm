#import <Cocoa/Cocoa.h>
#import <dispatch/dispatch.h>

#include <corral/CBPortal.h>
#include <corral/corral.h>

#include <chrono>
#include <functional>
#include <string>
#include <tuple>
#include <utility>

using namespace std::chrono_literals;

// ============================================================
// Minimal shared app state
// ============================================================

enum class EventKind {
    Click,
    Quit,
};

struct AppState {
    bool hovered = false;
    bool pressed = false;
    bool busy = false;
    bool quit_requested = false;
    std::string label = "Click me";

    NSView* view = nil;
    NSWindow* window = nil;

    // Installed while the root task is alive.
    std::function<void()> emit_click;
    std::function<void()> emit_quit;
};

static AppState g_app;

static void request_redraw() {
    if (g_app.view) {
        [g_app.view setNeedsDisplay:YES];
    }
}

static bool request_quit() {
    g_app.hovered = false;
    g_app.pressed = false;

    if (g_app.quit_requested) {
        return false;
    }
    g_app.quit_requested = true;

    if (g_app.emit_quit) {
        auto emit_quit = g_app.emit_quit;
        dispatch_async(dispatch_get_main_queue(), ^{
          emit_quit();
        });
        return true;
    }
    return false;
}

// ============================================================
// Corral event loop adapter for NSApplication
// ============================================================

struct CocoaLoop {
    NSApplication* app;
};

namespace corral {

// corral::run(eventLoop, task) expects EventLoopTraits with
// eventLoopID(), run(), and stop().
template <>
struct EventLoopTraits<CocoaLoop> {
    static corral::EventLoopID eventLoopID(CocoaLoop& loop) noexcept {
        return corral::EventLoopID{(__bridge const void*)loop.app};
    }

    static void run(CocoaLoop& loop) { [loop.app run]; }

    static void stop(CocoaLoop& loop) {
        // Wake the run loop after stop:, otherwise AppKit can remain blocked
        // waiting for the next event.
        dispatch_async(dispatch_get_main_queue(), ^{
          [loop.app stop:nil];
          NSEvent* wake = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                             location:NSZeroPoint
                                        modifierFlags:0
                                            timestamp:0
                                         windowNumber:0
                                              context:nil
                                              subtype:0
                                                data1:0
                                                data2:0];
          [loop.app postEvent:wake atStart:NO];
        });
    }
};

}  // namespace corral

// ============================================================
// Corral awaitables
// ============================================================

static corral::Task<void> sleep_on_main(std::chrono::milliseconds delay) {
    corral::CBPortal<> portal;
    co_await corral::untilCBCalled(
        [delay](auto cb) {
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, delay.count() * NSEC_PER_MSEC),
                           dispatch_get_main_queue(), ^{
                             cb();
                           });
        },
        portal);
}

static corral::Task<void> animate_click() {
    g_app.busy = true;
    g_app.pressed = true;
    g_app.label = "Working...";
    request_redraw();

    co_await sleep_on_main(800ms);

    g_app.pressed = false;
    g_app.label = "Done. Click again";
    request_redraw();

    co_await sleep_on_main(700ms);

    g_app.busy = false;
    g_app.label = "Click me";
    request_redraw();
}

// Long-lived root task.
// It installs callback bridges once, then awaits the portal repeatedly.
// Because CBPortal has only one callback's worth of buffering, this code
// backpressures clicks by ignoring them while busy.
static corral::Task<void> ui_main() {
    corral::CBPortal<EventKind> portal;

    auto event = co_await corral::untilCBCalled(
        [](auto cb) {
            g_app.emit_click = [cb]() mutable { cb(EventKind::Click); };
            g_app.emit_quit = [cb]() mutable { cb(EventKind::Quit); };
        },
        portal);

    while (true) {
        if (event == EventKind::Quit) {
            break;
        }

        if (event == EventKind::Click) {
            co_await animate_click();
        }

        event = co_await portal;
    }

    // Prevent callbacks after portal destruction.
    g_app.emit_click = nullptr;
    g_app.emit_quit = nullptr;
}

// ============================================================
// Minimal Cocoa view: custom button, no NSButton
// ============================================================

@interface CanvasView : NSView
@end

@implementation CanvasView

- (BOOL)isFlipped {
    return YES;
}

- (void)drawRect:(NSRect)dirtyRect {
    (void)dirtyRect;

    [[NSColor windowBackgroundColor] setFill];
    NSRectFill(self.bounds);

    const NSRect buttonRect = NSMakeRect(40, 40, 180, 56);

    NSColor* fill = nil;
    if (g_app.pressed) {
        fill = [NSColor colorWithCalibratedRed:0.25 green:0.45 blue:0.90 alpha:1.0];
    } else if (g_app.hovered) {
        fill = [NSColor colorWithCalibratedRed:0.35 green:0.55 blue:0.95 alpha:1.0];
    } else {
        fill = [NSColor colorWithCalibratedRed:0.30 green:0.50 blue:0.92 alpha:1.0];
    }

    NSBezierPath* path = [NSBezierPath bezierPathWithRoundedRect:buttonRect xRadius:12 yRadius:12];
    [fill setFill];
    [path fill];

    NSDictionary* attrs = @{
        NSFontAttributeName : [NSFont systemFontOfSize:16 weight:NSFontWeightMedium],
        NSForegroundColorAttributeName : [NSColor whiteColor]
    };

    NSString* text = [NSString stringWithUTF8String:g_app.label.c_str()];
    NSSize textSize = [text sizeWithAttributes:attrs];
    NSPoint p =
        NSMakePoint(buttonRect.origin.x + (buttonRect.size.width - textSize.width) * 0.5,
                    buttonRect.origin.y + (buttonRect.size.height - textSize.height) * 0.5);
    [text drawAtPoint:p withAttributes:attrs];
}

- (void)updateTrackingAreas {
    [super updateTrackingAreas];

    for (NSTrackingArea* area in [self trackingAreas]) {
        [self removeTrackingArea:area];
    }

    NSTrackingArea* area = [[NSTrackingArea alloc]
        initWithRect:self.bounds
             options:NSTrackingMouseMoved | NSTrackingMouseEnteredAndExited |
                     NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect
               owner:self
            userInfo:nil];
    [self addTrackingArea:area];
}

- (void)mouseMoved:(NSEvent*)event {
    NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
    NSRect buttonRect = NSMakeRect(40, 40, 180, 56);
    bool inside = NSPointInRect(p, buttonRect);

    if (inside != g_app.hovered) {
        g_app.hovered = inside;
        [self setNeedsDisplay:YES];
    }
}

- (void)mouseExited:(NSEvent*)event {
    (void)event;
    if (g_app.hovered) {
        g_app.hovered = false;
        [self setNeedsDisplay:YES];
    }
}

- (void)mouseDown:(NSEvent*)event {
    NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
    NSRect buttonRect = NSMakeRect(40, 40, 180, 56);

    if (NSPointInRect(p, buttonRect) && !g_app.busy && g_app.emit_click) {
        g_app.emit_click();
    }
}

@end

// ============================================================
// Minimal delegate: translate Cocoa shutdown into portal events
// ============================================================

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@end

@implementation AppDelegate

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
    (void)sender;

    // Turn Cmd-Q / app termination into a structured quit event.
    if (g_app.quit_requested || request_quit()) {
        return NSTerminateCancel;
    }
    return NSTerminateNow;
}

- (BOOL)windowShouldClose:(NSWindow*)sender {
    (void)sender;

    request_quit();
    return NO;
}

@end

// ============================================================
// Setup
// ============================================================

static void create_window(AppDelegate* delegate) {
    NSRect frame = NSMakeRect(0, 0, 320, 160);
    NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                       NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;

    NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:style
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];

    window.title = @"NSApp + corral::Task";
    window.delegate = delegate;

    CanvasView* view = [[CanvasView alloc] initWithFrame:frame];
    g_app.view = view;
    g_app.window = window;

    window.contentView = view;
    [window center];
    [window makeKeyAndOrderFront:nil];
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];

        AppDelegate* delegate = [AppDelegate new];
        app.delegate = delegate;

        create_window(delegate);
        [app activateIgnoringOtherApps:YES];

        CocoaLoop loop{app};
        corral::run(loop, ui_main());

        g_app.view = nil;
        g_app.window = nil;
        app.delegate = nil;
    }

    return 0;
}
