#import <Cocoa/Cocoa.h>

#include "experiments/coroutines/app.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace app::internal {

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

struct TaskScopeState {
    explicit TaskScopeState(UiLoop& loop)
        : nursery(std::make_unique<corral::UnsafeNursery>(loop)) {}

    std::unique_ptr<corral::UnsafeNursery> nursery;
};

struct ButtonState {
    explicit ButtonState(std::string button_title) : title(std::move(button_title)) {}

    std::string title;
    std::function<void()> on_click;
    NSButton* control = nil;
};

struct AppState;

struct WindowState : std::enable_shared_from_this<WindowState> {
    WindowState(std::shared_ptr<AppState> app_state, WindowOptions window_options);

    void set_title(std::string title);
    void add_button(const std::shared_ptr<ButtonState>& button);
    bool should_close();
    void did_close();

    std::shared_ptr<AppState> app;
    WindowOptions options;
    std::shared_ptr<TaskScopeState> task_scope;
    std::vector<std::shared_ptr<ButtonState>> buttons;
    std::function<bool()> on_close_request;
    NSWindow* window = nil;
    NSStackView* stack_view = nil;
};

struct AppState : std::enable_shared_from_this<AppState> {
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
        co_await corral::untilCBCalled(
            [this](Callback cb) {
                emit_quit = [this, cb]() mutable {
                    post_to_run_loop(loop.run_loop, [cb]() mutable { cb(); });
                };
            },
            [this] { emit_quit = nullptr; });
        emit_quit = nullptr;
    }

    void remove_window(NSWindow* window) {
        std::erase_if(windows, [window](const std::shared_ptr<WindowState>& state) {
            return state->window == window || state->window == nil;
        });
    }

    UiLoop loop;
    bool quit_requested = false;
    std::function<void()> emit_quit;
    std::vector<std::shared_ptr<WindowState>> windows;
    NSObject* delegate = nil;
};

void WindowState::set_title(std::string title) {
    if (window) {
        window.title = [NSString stringWithUTF8String:title.c_str()];
    }
}

void WindowState::add_button(const std::shared_ptr<ButtonState>& button) {
    if (!stack_view || !button) {
        return;
    }
    buttons.push_back(button);
}

bool WindowState::should_close() {
    if (on_close_request) {
        return on_close_request();
    }
    return true;
}

void WindowState::did_close() {
    task_scope->nursery->cancel();
    window = nil;
    stack_view = nil;
}

}  // namespace app::internal

namespace corral {

template <>
struct EventLoopTraits<app::internal::UiLoop> {
    static corral::EventLoopID eventLoopID(app::internal::UiLoop& loop) noexcept {
        return corral::EventLoopID{(__bridge const void*)loop.app};
    }

    static void run(app::internal::UiLoop& loop) { [loop.app run]; }

    static void stop(app::internal::UiLoop& loop) {
        app::internal::post_to_run_loop(loop.run_loop, [app = loop.app] {
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

using app::internal::AppState;
using app::internal::ButtonState;
using app::internal::WindowState;

@interface AppButtonControl : NSButton {
@public
    void* cpp_state;
}
@end

@implementation AppButtonControl
@end

@interface AppButtonDispatcher : NSObject
+ (instancetype)shared;
- (void)button_pressed:(id)sender;
@end

@implementation AppButtonDispatcher

+ (instancetype)shared {
    static AppButtonDispatcher* dispatcher = [AppButtonDispatcher new];
    return dispatcher;
}

- (void)button_pressed:(id)sender {
    auto* control = static_cast<AppButtonControl*>(sender);
    auto* state = static_cast<ButtonState*>(control->cpp_state);
    if (state && state->on_click) {
        state->on_click();
    }
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate> {
@public
    void* cpp_state;
}
@end

@implementation AppDelegate

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    (void)sender;
    return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
    (void)sender;
    auto* state = static_cast<AppState*>(cpp_state);
    if (state && (state->quit_requested || state->request_quit())) {
        return NSTerminateCancel;
    }
    return NSTerminateNow;
}

- (BOOL)windowShouldClose:(NSWindow*)sender {
    auto* state = static_cast<AppState*>(cpp_state);
    if (!state) {
        return YES;
    }

    for (const auto& window_state : state->windows) {
        if (window_state->window == sender) {
            return window_state->should_close();
        }
    }
    return YES;
}

- (void)windowWillClose:(NSNotification*)notification {
    auto* state = static_cast<AppState*>(cpp_state);
    if (!state) {
        return;
    }

    NSWindow* sender = notification.object;
    for (const auto& window_state : state->windows) {
        if (window_state->window == sender) {
            window_state->did_close();
            break;
        }
    }
    state->remove_window(sender);
}

@end

namespace {

static NSMenu* create_main_menu() {
    NSMenu* main_menu = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem* app_item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    NSMenu* app_menu = [[NSMenu alloc] initWithTitle:@""];
    [app_menu addItem:[[NSMenuItem alloc] initWithTitle:@"Quit"
                                                 action:@selector(terminate:)
                                          keyEquivalent:@"q"]];
    app_item.submenu = app_menu;
    [main_menu addItem:app_item];
    return main_menu;
}

static NSStackView* create_stack_view() {
    NSStackView* stack_view = [NSStackView stackViewWithViews:@[]];
    stack_view.orientation = NSUserInterfaceLayoutOrientationVertical;
    stack_view.alignment = NSLayoutAttributeLeading;
    stack_view.spacing = 12;
    stack_view.translatesAutoresizingMaskIntoConstraints = NO;
    stack_view.edgeInsets = NSEdgeInsetsMake(24, 24, 24, 24);
    return stack_view;
}

static NSWindow* create_native_window(const std::shared_ptr<WindowState>& state, id delegate) {
    NSRect frame = NSMakeRect(0, 0, state->options.width, state->options.height);
    NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                       NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;

    NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:style
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    window.delegate = delegate;

    NSView* content = [[NSView alloc] initWithFrame:frame];
    content.translatesAutoresizingMaskIntoConstraints = NO;
    NSStackView* stack_view = create_stack_view();

    [content addSubview:stack_view];
    [NSLayoutConstraint activateConstraints:@[
        [stack_view.leadingAnchor constraintEqualToAnchor:content.leadingAnchor],
        [stack_view.trailingAnchor constraintLessThanOrEqualToAnchor:content.trailingAnchor],
        [stack_view.topAnchor constraintEqualToAnchor:content.topAnchor],
    ]];

    window.contentView = content;
    [window center];
    [window makeKeyAndOrderFront:nil];

    state->stack_view = stack_view;
    return window;
}

}  // namespace

namespace app::internal {

WindowState::WindowState(std::shared_ptr<AppState> app_state, WindowOptions window_options)
    : app(std::move(app_state)),
      options(window_options),
      task_scope(std::make_shared<TaskScopeState>(app->loop)) {}

}  // namespace app::internal

namespace app {

TaskScope::TaskScope(std::shared_ptr<internal::TaskScopeState> state) : state_(std::move(state)) {}

void TaskScope::cancel() const {
    if (state_ && state_->nursery) {
        state_->nursery->cancel();
    }
}

void TaskScope::start_impl(std::function<corral::Task<void>()> task_factory) const {
    if (!state_ || !state_->nursery) {
        return;
    }

    state_->nursery->start(
        [task_factory = std::move(task_factory)]() mutable -> corral::Task<void> {
            co_await task_factory();
        });
}

Button::Button(std::shared_ptr<internal::ButtonState> state) : state_(std::move(state)) {}

Button Button::create(std::string title) {
    return Button(std::make_shared<internal::ButtonState>(std::move(title)));
}

void Button::on_click(std::function<void()> handler) const {
    if (state_) {
        state_->on_click = std::move(handler);
    }
}

Window::Window(std::shared_ptr<internal::WindowState> state) : state_(std::move(state)) {}

void Window::set_title(std::string title) const {
    if (state_) {
        state_->set_title(std::move(title));
    }
}

void Window::add(Button button) const {
    if (!state_ || !button.state_) {
        return;
    }

    AppButtonControl* control = [AppButtonControl buttonWithTitle:@(button.state_->title.c_str())
                                                           target:[AppButtonDispatcher shared]
                                                           action:@selector(button_pressed:)];
    control->cpp_state = button.state_.get();
    button.state_->control = control;
    [state_->stack_view addArrangedSubview:control];
    state_->buttons.push_back(std::move(button.state_));
}

TaskScope Window::task_scope() const {
    if (!state_) {
        return TaskScope();
    }
    return TaskScope(state_->task_scope);
}

void Window::on_close_request(std::function<bool()> handler) const {
    if (state_) {
        state_->on_close_request = std::move(handler);
    }
}

void Window::on_close(std::function<corral::Task<void>()> handler) const {
    TaskScope scope = task_scope();
    on_close_request([scope = std::move(scope), handler = std::move(handler)]() mutable {
        scope.start([handler = std::move(handler)]() mutable { return handler(); });
        return true;
    });
}

App::App(std::shared_ptr<internal::AppState> state) : state_(std::move(state)) {}

Window App::create_window(WindowOptions options) {
    auto window_state = std::make_shared<internal::WindowState>(state_, options);
    window_state->window = create_native_window(window_state, state_->delegate);
    state_->windows.push_back(window_state);
    return Window(window_state);
}

void App::run() {
    if (!state_) {
        return;
    }

    internal::g_current_ui_loop = &state_->loop;
    corral::run(state_->loop, state_->run_until_quit());
    internal::g_current_ui_loop = nullptr;

    state_->delegate = nil;
    state_->loop.app.delegate = nil;
}

App create_app() {
    auto state = std::make_shared<internal::AppState>();

    state->loop.app.activationPolicy = NSApplicationActivationPolicyRegular;
    state->loop.app.mainMenu = create_main_menu();

    AppDelegate* delegate = [AppDelegate new];
    delegate->cpp_state = state.get();
    state->delegate = delegate;
    state->loop.app.delegate = delegate;

    return App(std::move(state));
}

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

}  // namespace app
