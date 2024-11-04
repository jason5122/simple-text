#include "app/cocoa/impl_cocoa.h"
#include "app/window.h"
#include "util/std_print.h"

namespace app {

Window::Window(App& app, int width, int height) : pimpl{new impl{}} {
    NSRect frame = NSMakeRect(0, 1000, width, height);

    // TODO: Debug; remove this.
    // NSRect frame = NSScreen.mainScreen.visibleFrame;
    // frame.origin.y = 300;
    // frame.size.height -= 300;
    // frame.size.width -= 300;

    DisplayGL* displaygl = app.pimpl->displaygl.get();
    pimpl->window_controller = [[WindowController alloc] initWithFrame:frame
                                                             appWindow:this
                                                             displayGl:displaygl];

    // Implement window cascading.
    // if (NSEqualPoints(parent.pimpl->cascading_point, NSZeroPoint)) {
    //     NSPoint point = pimpl->ns_window.frame.origin;
    //     parent.pimpl->cascading_point = [pimpl->ns_window cascadeTopLeftFromPoint:point];

    //     [pimpl->ns_window center];
    // } else {
    //     parent.pimpl->cascading_point =
    //         [pimpl->ns_window cascadeTopLeftFromPoint:parent.pimpl->cascading_point];
    // }
}

Window::~Window() {
    [pimpl->window_controller release];
}

void Window::show() {
    [pimpl->window_controller show];
}

void Window::close() {
    [pimpl->window_controller close];
}

void Window::redraw() {
    [pimpl->window_controller redraw];
}

int Window::width() {
    return [pimpl->window_controller getWidth];
}

int Window::height() {
    return [pimpl->window_controller getHeight];
}

int Window::scaleFactor() {
    return [pimpl->window_controller getScaleFactor];
}

bool Window::isDarkMode() {
    return [pimpl->window_controller isDarkMode];
}

void Window::setTitle(const std::string& title) {
    [pimpl->window_controller setTitle:title];
}

void Window::setFilePath(fs::path path) {
    [pimpl->window_controller setFilePath:path];
}

std::optional<std::string> Window::openFilePicker() {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    panel.title = @"Choose File";
    panel.prompt = @"Choose";
    panel.canChooseDirectories = false;
    panel.canChooseFiles = true;

    // TODO: Make this a parameter.
    // TODO: Expand tilde paths in a helper function (probably in //base/filesystem/) and implement
    // for each OS.
    // panel.directoryURL = [NSURL fileURLWithPath:@"~".stringByExpandingTildeInPath];

    if ([panel runModal] != NSModalResponseCancel) {
        return panel.URL.fileSystemRepresentation;
    } else {
        return std::nullopt;
    }

    // TODO: Investigate how to open a sheet synchronously.
    // [panel beginSheetModalForWindow:pimpl->window_controller.window
    //                   completionHandler:^(NSInteger result) {
    //                     if (result == NSModalResponseOK) {
    //                         std::println(panel.URL.fileSystemRepresentation);
    //                     }
    //                   }];
}

// TODO: De-duplicate code with GLView GetPosition().
std::optional<app::Point> Window::mousePosition() {
    int window_width = [pimpl->window_controller getWidth];
    int window_height = [pimpl->window_controller getHeight];
    NSWindow* ns_window = [pimpl->window_controller getNsWindow];

    NSPoint mouse_pos = ns_window.mouseLocationOutsideOfEventStream;
    mouse_pos.y = window_height - mouse_pos.y;  // Set origin at top left.

    if ((mouse_pos.x < 0 || mouse_pos.x > window_width - 1) ||
        (mouse_pos.y < 0 || mouse_pos.y > window_height - 1)) {
        return std::nullopt;
    }

    int mouse_x = std::round(mouse_pos.x);
    int mouse_y = std::round(mouse_pos.y);

    assert(!(mouse_x < 0 || mouse_x >= window_width));
    assert(!(mouse_y < 0 || mouse_y >= window_height));

    int scale = [pimpl->window_controller getScaleFactor];
    int scaled_mouse_x = mouse_x * scale;
    int scaled_mouse_y = mouse_y * scale;
    return app::Point{scaled_mouse_x, scaled_mouse_y};
}

// TODO: Check for negative values?
std::optional<app::Point> Window::mousePositionRaw() {
    NSPoint mouse_pos = NSEvent.mouseLocation;
    int mouse_x = std::round(mouse_pos.x);
    int mouse_y = std::round(mouse_pos.y);
    return app::Point{mouse_x, mouse_y};
}

}  // namespace app
