#include "experiments/gui_api_redesign/app.h"
#include "experiments/gui_api_redesign/platform/mac/gl_view.h"
#include <AppKit/AppKit.h>

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
    auto win = std::make_unique<Window>(width, height);

    NSRect frame = NSMakeRect(0, 0, 1200, 800);
    NSUInteger style =
        NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
    NSWindow* window = [[[NSWindow alloc] initWithContentRect:frame
                                                    styleMask:style
                                                      backing:NSBackingStoreBuffered
                                                        defer:false] autorelease];
    window.contentView = [[[GLView alloc] initWithFrame:frame appWindow:win.get()] autorelease];

    [window setTitle:@"GUI API Redesign"];
    [window makeKeyAndOrderFront:nil];

    Window& ref = *win;
    windows_.push_back(std::move(win));
    return ref;
}
