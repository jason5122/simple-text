#include "base/check.h"
#include "platform/cocoa/app_mac.h"
#include "platform/cocoa/window_mac.h"
#include "platform/task_internal.h"
#include <Cocoa/Cocoa.h>
#include <algorithm>
#include <utility>

namespace platform::internal {

struct UiLoop {
    NSApplication* app = nil;
    CFRunLoopRef run_loop = nullptr;
};

static thread_local UiLoop* g_current_ui_loop = nullptr;

static void post_to_run_loop(CFRunLoopRef run_loop, std::function<void()> fn) {
    CFRunLoopPerformBlock(run_loop, kCFRunLoopCommonModes, ^{
      fn();
    });
    CFRunLoopWakeUp(run_loop);
}

}  // namespace platform::internal

namespace corral {

template <>
struct EventLoopTraits<platform::internal::UiLoop> {
    static corral::EventLoopID eventLoopID(platform::internal::UiLoop& loop) noexcept {
        return corral::EventLoopID{(__bridge const void*)loop.app};
    }

    static void run(platform::internal::UiLoop& loop) { [loop.app run]; }

    static void stop(platform::internal::UiLoop& loop) {
        platform::internal::post_to_run_loop(loop.run_loop, [app = loop.app] {
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

@interface PlatformAppDelegate : NSObject <NSApplicationDelegate> {
@public
    void* app;
}
@end

@implementation PlatformAppDelegate

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    return YES;
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication*)sender {
    return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
    auto* platform_app = static_cast<platform::AppMac*>(app);
    if (platform_app && platform_app->request_quit()) {
        return NSTerminateCancel;
    }
    return NSTerminateNow;
}

@end

namespace {

void build_main_menu() {
    NSMenu* main_menu = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem* app_item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    NSMenu* app_menu = [[NSMenu alloc] initWithTitle:@""];
    [app_menu addItem:[[NSMenuItem alloc] initWithTitle:@"Quit"
                                                 action:@selector(terminate:)
                                          keyEquivalent:@"q"]];
    app_item.submenu = app_menu;
    [main_menu addItem:app_item];
    NSApp.mainMenu = main_menu;
}

}  // namespace

namespace platform {

AppMac::AppMac(AppOptions options)
    : options_(options),
      ui_loop_(std::make_unique<internal::UiLoop>()),
      task_scope_(std::make_shared<internal::TaskScopeState>()) {}

AppMac::~AppMac() {
    if (NSApp.delegate == (__bridge id)app_delegate_) {
        NSApp.delegate = nil;
    }
}

std::unique_ptr<App> AppMac::create(AppOptions options) {
    CHECK_EQ(RendererBackend::kOpenGL, options.renderer_backend);

    [NSApplication sharedApplication];
    NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
    NSWindow.allowsAutomaticWindowTabbing = NO;
    build_main_menu();

    auto app = std::unique_ptr<AppMac>(new AppMac(options));
    app->ui_loop_->app = NSApp;
    app->ui_loop_->run_loop = CFRunLoopGetCurrent();

    PlatformAppDelegate* delegate = [PlatformAppDelegate new];
    delegate->app = app.get();
    app->app_delegate_ = (__bridge void*)delegate;
    NSApp.delegate = delegate;
    return app;
}

Window* AppMac::create_window(WindowOptions options, WindowDelegate* delegate) {
    auto window = WindowMac::create(*this, std::move(options), delegate);
    WindowMac* raw_window = window.get();
    windows_.push_back(std::move(window));
    if (app_nursery_) {
        app_nursery_->start([this, raw_window]() -> corral::Task<void> {
            co_await raw_window->run_until_closed();
            window_did_close(raw_window);
        });
    }
    return raw_window;
}

TaskScope AppMac::task_scope() const { return TaskScope(task_scope_); }

int AppMac::run() {
    internal::g_current_ui_loop = ui_loop_.get();
    corral::run(*ui_loop_, run_until_quit());
    internal::g_current_ui_loop = nullptr;
    return 0;
}

void AppMac::quit() { request_quit(); }

bool AppMac::request_quit() {
    if (quit_requested_) {
        return true;
    }
    quit_requested_ = true;
    if (emit_quit_) {
        emit_quit_();
    }
    return true;
}

corral::Task<void> AppMac::run_until_quit() {
    using Callback = corral::CBPortal<>::Callback;
    auto await_quit = corral::untilCBCalled(
        [this](Callback cb) {
            emit_quit_ = [this, cb]() mutable {
                internal::post_to_run_loop(ui_loop_->run_loop, [cb]() mutable { cb(); });
            };
            if (quit_requested_) {
                emit_quit_();
            }
        },
        [this] { emit_quit_ = nullptr; });

    CORRAL_WITH_NURSERY(nursery) {
        app_nursery_ = &nursery;
        task_scope_->bind(nursery);
        for (const auto& window : windows_) {
            WindowMac* raw_window = window.get();
            nursery.start([this, raw_window]() -> corral::Task<void> {
                co_await raw_window->run_until_closed();
                window_did_close(raw_window);
            });
        }
        co_await std::move(await_quit);
        task_scope_->unbind(nursery);
        app_nursery_ = nullptr;
        co_return corral::cancel;
    };

    task_scope_->nursery = nullptr;
    app_nursery_ = nullptr;
    emit_quit_ = nullptr;
}

void AppMac::window_requested_close(WindowMac* window) {
    if (!window) {
        return;
    }
    window->did_close();
}

void AppMac::window_did_close(WindowMac* window) {
    std::erase_if(windows_, [window](const std::unique_ptr<WindowMac>& candidate) {
        return candidate.get() == window;
    });
}

}  // namespace platform

namespace platform {

corral::Task<void> resume_on_ui() {
    if (!internal::g_current_ui_loop) {
        co_return;
    }

    using Callback = corral::CBPortal<>::Callback;
    internal::UiLoop* loop = internal::g_current_ui_loop;
    co_await corral::untilCBCalled([run_loop = loop->run_loop](Callback cb) {
        internal::post_to_run_loop(run_loop, [cb]() mutable { cb(); });
    });
}

corral::Task<void> sleep_for(std::chrono::milliseconds delay) {
    if (!internal::g_current_ui_loop) {
        co_return;
    }

    struct TimerState {
        CFRunLoopTimerRef timer = nullptr;
    };

    using Callback = corral::CBPortal<>::Callback;
    CFRunLoopRef run_loop = internal::g_current_ui_loop->run_loop;
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

}  // namespace platform
