#import <Cocoa/Cocoa.h>

#include <corral/CBPortal.h>
#include <corral/corral.h>

#include <chrono>
#include <functional>
#include <memory>
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

static void post_to_run_loop(CFRunLoopRef run_loop, std::function<void()> fn) {
    CFRunLoopPerformBlock(run_loop, kCFRunLoopCommonModes, ^{
      fn();
    });
    CFRunLoopWakeUp(run_loop);
}

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
        g_app.emit_quit();
        return true;
    }
    return false;
}

// ============================================================
// Corral event loop adapter for NSApplication
// ============================================================

struct CocoaLoop {
    NSApplication* app;
    CFRunLoopRef run_loop;
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
        post_to_run_loop(loop.run_loop, [app = loop.app] {
            [app stop:nil];
            NSEvent* wake = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                               location:NSZeroPoint
                                          modifierFlags:0
                                              timestamp:0
                                           windowNumber:0
                                                context:nil
                                                subtype:0
                                                  data1:0
                                                  data2:0];
            [app postEvent:wake atStart:NO];
        });
    }
};

}  // namespace corral

// ============================================================
// Corral awaitables
// ============================================================

static corral::Task<void> sleep_on_main(CFRunLoopRef run_loop, std::chrono::milliseconds delay) {
    struct TimerState {
        CFRunLoopTimerRef timer = nullptr;
    };

    using Callback = corral::CBPortal<>::Callback;
    auto state = std::make_shared<TimerState>();
    co_await corral::untilCBCalled(
        [delay, run_loop, state](Callback cb) mutable {
            CFAbsoluteTime fire_at =
                CFAbsoluteTimeGetCurrent() + (static_cast<double>(delay.count()) / 1000.0);
            state->timer = CFRunLoopTimerCreateWithHandler(nullptr, fire_at, 0, 0, 0,
                                                           ^(CFRunLoopTimerRef timer) {
                                                             if (state->timer == timer) {
                                                                 state->timer = nullptr;
                                                             }
                                                             CFRunLoopTimerInvalidate(timer);
                                                             CFRelease(timer);
                                                             cb();
                                                           });
            CFRunLoopAddTimer(run_loop, state->timer, kCFRunLoopCommonModes);
            CFRunLoopWakeUp(run_loop);
        },
        [state] {
            if (state->timer) {
                CFRunLoopTimerInvalidate(state->timer);
                CFRelease(state->timer);
                state->timer = nullptr;
            }
        });
}

static corral::Task<void> animate_click(CFRunLoopRef run_loop) {
    g_app.busy = true;
    g_app.pressed = true;
    g_app.label = "Working...";
    request_redraw();

    co_await sleep_on_main(run_loop, 800ms);

    g_app.pressed = false;
    g_app.label = "Done. Click again";
    request_redraw();

    co_await sleep_on_main(run_loop, 700ms);

    g_app.busy = false;
    g_app.label = "Click me";
    request_redraw();
}

static void reset_button_state() {
    g_app.busy = false;
    g_app.pressed = false;
    g_app.hovered = false;
    g_app.label = "Click me";
    request_redraw();
}

// Long-lived root task.
// It installs callback bridges once, then awaits the portal repeatedly.
// Because CBPortal has only one callback's worth of buffering, this code
// backpressures clicks by ignoring them while busy.
static corral::Task<void> ui_main() {
    corral::CBPortal<EventKind> portal;
    CFRunLoopRef run_loop = CFRunLoopGetCurrent();

    auto event = co_await corral::untilCBCalled(
        [run_loop](auto cb) {
            g_app.emit_click = [cb]() mutable { cb(EventKind::Click); };
            g_app.emit_quit = [run_loop, cb]() mutable {
                post_to_run_loop(run_loop, [cb]() mutable { cb(EventKind::Quit); });
            };
        },
        portal);

    while (true) {
        if (event == EventKind::Quit) {
            break;
        }

        if (event == EventKind::Click) {
            auto [animation_done, next_event] =
                co_await corral::anyOf(animate_click(run_loop), portal);
            if (next_event.has_value()) {
                event = *next_event;
                if (event == EventKind::Quit) {
                    reset_button_state();
                }
                continue;
            }

            if (animation_done.has_value()) {
                event = co_await portal;
                continue;
            }
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

int main() {
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        app.activationPolicy = NSApplicationActivationPolicyRegular;

        AppDelegate* delegate = [AppDelegate new];
        app.delegate = delegate;

        NSMenu* main_menu = [[NSMenu alloc] initWithTitle:@""];
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
        NSMenu* submenu = [[NSMenu alloc] initWithTitle:@""];
        [submenu addItem:[[NSMenuItem alloc] initWithTitle:@"Quit"
                                                    action:@selector(terminate:)
                                             keyEquivalent:@"q"]];
        item.submenu = submenu;
        [main_menu addItem:item];
        NSApp.mainMenu = main_menu;

        create_window(delegate);
        [app activateIgnoringOtherApps:YES];

        CocoaLoop loop{app, CFRunLoopGetCurrent()};
        corral::run(loop, ui_main());

        g_app.view = nil;
        g_app.window = nil;
        app.delegate = nil;
    }
}
