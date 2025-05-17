#include "experiments/gui_api_redesign/platform/cocoa/gl_view.h"
#include "experiments/gui_api_redesign/window.h"
#include <AppKit/AppKit.h>

struct Window::Impl {};

Window::~Window() = default;
Window::Window() : pimpl_(std::make_unique<Impl>()) {}

std::unique_ptr<Window> Window::create(int width, int height, void* cx) {
    auto* mgr = static_cast<GLContextManager*>(cx);

    auto window = std::unique_ptr<Window>(new Window());
    NSRect frame = NSMakeRect(0, 0, width, height);
    NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                       NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
    NSWindow* ns_window = [[[NSWindow alloc] initWithContentRect:frame
                                                       styleMask:style
                                                         backing:NSBackingStoreBuffered
                                                           defer:false] autorelease];
    ns_window.contentView = [[[GLView alloc] initWithFrame:frame
                                                 appWindow:window.get()
                                          glContextManager:mgr] autorelease];
    [ns_window setTitle:@"GUI API Redesign"];
    [ns_window makeKeyAndOrderFront:nil];
    return window;
}
