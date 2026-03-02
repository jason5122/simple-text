#include "app/cocoa/app_controller.h"
#include "app/cocoa/gl_app.h"
#include "app/cocoa/gl_window.h"
#include "base/check.h"
#include <Cocoa/Cocoa.h>

namespace app {

GLApp::GLApp() = default;

std::unique_ptr<App> GLApp::create() {
    [NSApplication sharedApplication];
    // TODO: Call this in `applicationWillFinishLaunching` like Chromium.
    NSWindow.allowsAutomaticWindowTabbing = NO;

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
    // Create the app delegate by requesting the shared AppController.
    CHECK_EQ(nil, NSApp.delegate);
    AppController* app_controller = AppController.sharedController;
    CHECK_NE(nil, NSApp.delegate);

    @autoreleasepool {
        [NSApp run];
    }
}

std::unique_ptr<Window> GLApp::create_window(const WindowOptions& options) {
    return std::unique_ptr<Window>(GLWindow::create(options));
}

}  // namespace app
