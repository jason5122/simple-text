#include <Cocoa/Cocoa.h>
#include <QuartzCore/QuartzCore.h>
#include <corral/corral.h>
#include <functional>
#include <print>

namespace {

constexpr bool kBenchmarkMode = true;

struct UiLoop {
    NSApplication* app = nil;
    CFRunLoopRef run_loop = nullptr;
};

static void post_to_run_loop(CFRunLoopRef run_loop, std::function<void()> fn) {
    CFRunLoopPerformBlock(run_loop, kCFRunLoopCommonModes, ^{
      fn();
    });
    CFRunLoopWakeUp(run_loop);
}

struct AppState {
    AppState() {
        loop.app = [NSApplication sharedApplication];
        loop.run_loop = CFRunLoopGetCurrent();
    }

    bool request_quit() {
        if (quit_requested) {
            return false;
        }
        quit_requested = true;
        if (emit_quit) {
            emit_quit();
            return true;
        }
        return false;
    }

    corral::Task<void> run_until_quit() {
        using Callback = corral::CBPortal<>::Callback;
        auto await_quit = corral::untilCBCalled(
            [this](Callback cb) {
                emit_quit = [cb]() mutable { cb(); };
                if (quit_requested) {
                    emit_quit();
                }
            },
            [this] { emit_quit = nullptr; });

        co_await std::move(await_quit);
    }

    UiLoop loop;
    bool quit_requested = false;
    std::function<void()> emit_quit;
};

static AppState* g_app_state = nullptr;

}  // namespace

@interface GLLayer : CAOpenGLLayer
@end

@implementation GLLayer
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
    std::println("draw");
    if constexpr (kBenchmarkMode) {
        g_app_state->request_quit();
    }
}
@end

namespace corral {

template <>
struct EventLoopTraits<UiLoop> {
    static corral::EventLoopID eventLoopID(UiLoop& loop) noexcept {
        return corral::EventLoopID{(__bridge const void*)loop.app};
    }

    static void run(UiLoop& loop) { [loop.app run]; }

    static void stop(UiLoop& loop) { [loop.app terminate:nil]; }
};

}  // namespace corral

int main(int argc, const char* argv[]) {
    @autoreleasepool {
        AppState app_state;
        g_app_state = &app_state;

        // Add Command+Q to quit.
        NSMenu* main_menu = [[NSMenu alloc] initWithTitle:@""];
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
        NSMenu* submenu = [[NSMenu alloc] initWithTitle:@""];
        [submenu addItem:[[NSMenuItem alloc] initWithTitle:@"Quit"
                                                    action:@selector(terminate:)
                                             keyEquivalent:@"q"]];
        item.submenu = submenu;
        [main_menu addItem:item];
        NSApp.mainMenu = main_menu;

        NSRect frame = NSMakeRect(0, 0, 1200, 800);
        NSUInteger style =
            NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
        NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                       styleMask:style
                                                         backing:NSBackingStoreBuffered
                                                           defer:false];

        NSView* gl_view = [[NSView alloc] initWithFrame:frame];
        gl_view.layer = [[GLLayer alloc] init];
        gl_view.layer.needsDisplayOnBoundsChange = true;
        window.contentView = gl_view;

        [window setTitle:@"Coroutine macOS App"];
        NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
        [window makeKeyAndOrderFront:nil];

        corral::run(app_state.loop, app_state.run_until_quit());
        g_app_state = nullptr;
    }
    return 0;
}
