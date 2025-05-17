#include "experiments/gui_api_redesign/app.h"
#include "experiments/gui_api_redesign/platform/cocoa/gl_context_manager.h"
#include <AppKit/AppKit.h>
#include <fmt/base.h>

struct App::Impl {
    std::unique_ptr<GLContextManager> mgr;
};

App::~App() = default;
App::App() : pimpl_(std::make_unique<Impl>()) {}

std::unique_ptr<App> App::create() {
    auto app = std::unique_ptr<App>(new App());
    auto mgr = GLContextManager::create();
    if (!mgr) {
        fmt::println(stderr, "Failed to initialize GL context manager");
        return nullptr;
    }
    app->pimpl_->mgr = std::move(mgr);
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

Window& App::create_window(int width, int height) {
    auto window = Window::create(width, height, static_cast<void*>(pimpl_->mgr.get()));
    Window& ref = *window;
    windows_.push_back(std::move(window));
    return ref;
}
