#include "experiments/gui_api_redesign/platform/mac/gl_view.h"
#include "experiments/gui_api_redesign/window.h"
#include <AppKit/AppKit.h>

struct Window::Impl {};

Window::~Window() = default;
Window::Window() : pimpl_(std::make_unique<Impl>()) {}

std::unique_ptr<Window> Window::create(int width, int height) {
    auto window = std::unique_ptr<Window>(new Window());
    NSRect frame = NSMakeRect(0, 0, width, height);
    NSUInteger style =
        NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
    NSWindow* ns_window = [[[NSWindow alloc] initWithContentRect:frame
                                                       styleMask:style
                                                         backing:NSBackingStoreBuffered
                                                           defer:false] autorelease];
    ns_window.contentView = [[[GLView alloc] initWithFrame:frame
                                                 appWindow:window.get()] autorelease];

    [ns_window setTitle:@"GUI API Redesign"];
    [ns_window makeKeyAndOrderFront:nil];
    return window;
}
