#include "app/cocoa/gl_window.h"
#include "base/strings/sys_string_conversions.h"

namespace app {

GLWindow::GLWindow(const WindowOptions& options) {
    NSRect frame = NSMakeRect(0, 0, options.width, options.height);
    NSUInteger style =
        NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;

    ns_window = [[NSWindow alloc] initWithContentRect:frame
                                            styleMask:style
                                              backing:NSBackingStoreBuffered
                                                defer:false];

    set_title(options.title);
    [ns_window makeKeyAndOrderFront:nil];
}

std::unique_ptr<Window> GLWindow::create(const WindowOptions& options) {
    return std::unique_ptr<Window>(new GLWindow(options));
}

void GLWindow::set_title(std::string_view title) {
    auto ns_title = base::sys_utf8_to_nsstring(title);
    [ns_window setTitle:ns_title];
}

}  // namespace app
