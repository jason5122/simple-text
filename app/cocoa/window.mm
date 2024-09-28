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
    NSOpenPanel* openPanel = [NSOpenPanel openPanel];
    openPanel.title = @"Choose File";
    openPanel.prompt = @"Choose";
    openPanel.canChooseDirectories = false;
    openPanel.canChooseFiles = true;
    if ([openPanel runModal] != NSModalResponseCancel) {
        return openPanel.URL.fileSystemRepresentation;
    } else {
        return std::nullopt;
    }

    // TODO: Investigate how to open a sheet synchronously.
    // [openPanel beginSheetModalForWindow:pimpl->window_controller.window
    //                   completionHandler:^(NSInteger result) {
    //                     if (result == NSModalResponseOK) {
    //                         std::println(openPanel.URL.fileSystemRepresentation);
    //                     }
    //                   }];
}

// TODO: De-duplicate code with GLView GetPosition().
std::pair<int, int> Window::mousePosition() {
    NSWindow* ns_window = [pimpl->window_controller getNsWindow];
    NSPoint mouse_pos = ns_window.mouseLocationOutsideOfEventStream;

    int mouse_x = std::round(mouse_pos.x);
    int mouse_y = std::round(mouse_pos.y);
    mouse_y = [pimpl->window_controller getHeight] - mouse_y;  // Set origin at top left.

    int scale = [pimpl->window_controller getScaleFactor];
    int scaled_mouse_x = mouse_x * scale;
    int scaled_mouse_y = mouse_y * scale;
    return {scaled_mouse_x, scaled_mouse_y};
}

}
