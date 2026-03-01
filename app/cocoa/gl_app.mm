#include "app/cocoa/gl_app.h"
#include "app/cocoa/gl_window.h"
#include <Cocoa/Cocoa.h>

namespace app {

GLApp::GLApp() = default;

std::unique_ptr<App> GLApp::create() {
    [NSApplication sharedApplication];
    NSApp.activationPolicy = NSApplicationActivationPolicyRegular;

    NSMenu* main_menu = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    NSMenu* submenu = [[NSMenu alloc] initWithTitle:@""];
    [submenu addItem:[[NSMenuItem alloc] initWithTitle:@"Quit"
                                                action:@selector(terminate:)
                                         keyEquivalent:@"q"]];
    item.submenu = submenu;
    [main_menu addItem:item];
    NSApp.mainMenu = main_menu;

    return std::unique_ptr<App>(new GLApp());
}

void GLApp::run() {
    @autoreleasepool {
        [NSApp run];
    }
}

std::unique_ptr<Window> GLApp::create_window(const WindowOptions& options) {
    return std::unique_ptr<Window>(GLWindow::create(options));
}

}  // namespace app
