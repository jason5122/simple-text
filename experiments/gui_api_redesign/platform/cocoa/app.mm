#include "experiments/gui_api_redesign/app.h"
#include "experiments/gui_api_redesign/platform/cocoa/window_create_info.h"
#include <AppKit/AppKit.h>

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

        [NSApp run];
    }
    return 0;  // TODO: How do we get non-zero return values from NSApp?
}

Window* App::create_window(int width, int height) {
    auto ctx = pimpl_->ctx->create_shared(*pimpl_->pf);
    if (!ctx) return nullptr;

    WindowCreateInfo info = {
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
