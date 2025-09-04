#include "experiments/gui_api_redesign/app.h"
#include "experiments/gui_api_redesign/platform/cocoa/window_create_info.h"
#include <AppKit/AppKit.h>

// TODO: Remove this.
// ============================================================================
#include "base/apple/scoped_cftyperef.h"
#include <spdlog/spdlog.h>
using base::apple::ScopedCFTypeRef;
#include <queue>
ScopedCFTypeRef<CFRunLoopSourceRef> work_source;
auto run_loop = ScopedCFTypeRef<CFRunLoopRef>(CFRunLoopGetCurrent());
std::queue<std::function<void()>> tasks;
std::mutex task_mutex;

static void task_source_perform(void* info) {
    std::queue<std::function<void()>> local;

    {
        std::lock_guard<std::mutex> lock(task_mutex);
        std::swap(local, tasks);
    }

    while (!local.empty()) {
        local.front()();
        local.pop();
    }
}

static void schedule_work(std::function<void()> fn) {
    {
        std::lock_guard<std::mutex> lock(task_mutex);
        tasks.push(std::move(fn));
    }
    CFRunLoopSourceSignal(work_source.get());
    CFRunLoopWakeUp(run_loop.get());
}
// ============================================================================

struct App::Impl {
    std::unique_ptr<GLContext> ctx;
    std::unique_ptr<GLPixelFormat> pf;
};

App::~App() = default;
App::App() : pimpl_(std::make_unique<Impl>()) {}

std::unique_ptr<App> App::create() {
    auto pf = GLPixelFormat::create();
    if (!pf) return nullptr;
    auto ctx = GLContext::create(*pf);
    if (!ctx) return nullptr;

    auto app = std::unique_ptr<App>(new App());
    app->pimpl_->ctx = std::move(ctx);
    app->pimpl_->pf = std::move(pf);
    return app;
}

int App::run() {
    @autoreleasepool {
        [NSApplication sharedApplication];

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

        // Run loop example.
        // TODO: Refactor this.
        // ============================================================================
        CFRunLoopSourceContext source_context = {};
        source_context.info = nullptr;
        source_context.perform = task_source_perform;
        work_source.reset(CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &source_context));
        CFRunLoopAddSource(run_loop.get(), work_source.get(), kCFRunLoopCommonModes);

        schedule_work([] { spdlog::info("scheduled work"); });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC),
                       dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
                         schedule_work([] { spdlog::info("delayed scheduled work"); });
                       });
        // ============================================================================

        [NSApp run];
    }
    return 0;  // TODO: How do we get non-zero return values from NSApp?
}

Window* App::create_window(int width, int height) {
    auto ctx = pimpl_->ctx->create_shared(*pimpl_->pf);
    if (!ctx) return nullptr;

    Window::CreateInfo info = {
        .width = width,
        .height = height,
        .ctx = std::move(ctx),
        .pf = pimpl_->pf.get(),
    };
    auto window = Window::create(std::move(info));
    if (!window) return nullptr;

    Window* ptr = window.get();
    windows_.push_back(std::move(window));
    return ptr;
}
