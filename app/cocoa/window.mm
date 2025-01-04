#include "app/window.h"

#include "app/cocoa/impl_cocoa.h"

// TODO: Debug use; remove this.
#include <fmt/base.h>

namespace app {

Window::Window(App& app, int width, int height) : pimpl{new impl{}} {
    NSRect frame = NSMakeRect(0, 1000, width, height);

    // TODO: Debug; remove this.
    // NSRect frame = NSScreen.mainScreen.visibleFrame;
    // frame.origin.y = 300;
    // frame.size.height -= 300;
    // frame.size.width -= 300;

    DisplayGL* display_gl = app.pimpl->display_gl.get();
    pimpl->window_controller = [[WindowController alloc] initWithFrame:frame
                                                             appWindow:this
                                                             displayGL:display_gl];

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
    [pimpl->window_controller invalidateAppWindowPointer];
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

int Window::width() const {
    return [pimpl->window_controller getWidth];
}

int Window::height() const {
    return [pimpl->window_controller getHeight];
}

int Window::scale() const {
    return [pimpl->window_controller getScaleFactor];
}

bool Window::isDarkMode() const {
    return [pimpl->window_controller isDarkMode];
}

void Window::setTitle(std::string_view title) {
    [pimpl->window_controller setTitle:title];
}

void Window::setFilePath(std::string_view path) {
    [pimpl->window_controller setFilePath:path];
}

std::optional<std::string> Window::openFilePicker() const {
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
    //                         fmt::println(panel.URL.fileSystemRepresentation);
    //                     }
    //                   }];
}

void Window::setCursorStyle(CursorStyle style) {
    if (style == CursorStyle::kArrow) {
        [NSCursor.arrowCursor set];
    } else if (style == CursorStyle::kIBeam) {
        [NSCursor.IBeamCursor set];
    } else if (style == CursorStyle::kResizeLeftRight) {
        [NSCursor.resizeLeftRightCursor set];
    } else if (style == CursorStyle::kResizeUpDown) {
        [NSCursor.resizeUpDownCursor set];
    }
}

}  // namespace app
