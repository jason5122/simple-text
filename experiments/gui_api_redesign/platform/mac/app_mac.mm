#include "experiments/gui_api_redesign/app.h"
#include "experiments/gui_api_redesign/platform/mac/gl_context_manager.h"
#include <AppKit/AppKit.h>
#include <fmt/base.h>

struct App::Impl {
    std::unique_ptr<GLContextManager> gl_context_manager;
};

App::~App() = default;

App::App() : pimpl_(std::make_unique<Impl>()) {}

bool App::initialize() {
    pimpl_->gl_context_manager = GLContextManager::create();
    if (!pimpl_->gl_context_manager) {
        fmt::println(stderr, "Failed to initialize GL context manager");
        return false;
    }
    return true;
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
    auto win = std::unique_ptr<Window>(new Window(width, height));
    win->initialize();

    Window& ref = *win;
    windows_.push_back(std::move(win));
    return ref;
}
