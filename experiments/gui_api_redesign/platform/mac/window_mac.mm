#include "experiments/gui_api_redesign/platform/mac/gl_view.h"
#include "experiments/gui_api_redesign/window.h"
#include <AppKit/AppKit.h>

struct Window::Impl {};

Window::~Window() = default;

Window::Window(int width, int height)
    : width_(width), height_(height), pimpl_(std::make_unique<Impl>()) {}

void Window::initialize() {
    NSRect frame = NSMakeRect(0, 0, width_, height_);
    NSUInteger style =
        NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
    NSWindow* window = [[[NSWindow alloc] initWithContentRect:frame
                                                    styleMask:style
                                                      backing:NSBackingStoreBuffered
                                                        defer:false] autorelease];
    window.contentView = [[[GLView alloc] initWithFrame:frame appWindow:this] autorelease];

    [window setTitle:@"GUI API Redesign"];
    [window makeKeyAndOrderFront:nil];
}
