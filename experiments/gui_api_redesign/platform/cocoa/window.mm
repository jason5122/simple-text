#include "experiments/gui_api_redesign/platform/cocoa/gl_view.h"
#include "experiments/gui_api_redesign/platform/cocoa/window_create_info.h"
#include "experiments/gui_api_redesign/window.h"
#include <AppKit/AppKit.h>

struct Window::Impl {
    std::unique_ptr<GLContext> ctx;
    GLPixelFormat* pf;
};

Window::~Window() = default;
Window::Window() : renderer_(Renderer::create()), pimpl_(std::make_unique<Impl>()) {}

std::unique_ptr<Window> Window::create(WindowCreateInfo info) {
    auto window = std::unique_ptr<Window>(new Window());
    window->pimpl_->ctx = std::move(info.ctx);
    window->pimpl_->pf = info.pf;

    NSRect frame = NSMakeRect(0, 0, info.width, info.height);
    NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                       NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
    NSWindow* ns_window = [[[NSWindow alloc] initWithContentRect:frame
                                                       styleMask:style
                                                         backing:NSBackingStoreBuffered
                                                           defer:false] autorelease];
    ns_window.contentView = [[[GLView alloc] initWithFrame:frame
                                                 appWindow:window.get()
                                                 glContext:window->pimpl_->ctx.get()
                                             glPixelFormat:window->pimpl_->pf] autorelease];
    [ns_window setTitle:@"GUI API Redesign"];
    [ns_window makeKeyAndOrderFront:nil];
    return window;
}
