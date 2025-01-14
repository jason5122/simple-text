#include "gui/platform/window_widget.h"

#include "gui/platform/cocoa/impl_cocoa.h"

// TODO: Debug use; remove this.
#include <fmt/base.h>

namespace gui {

WindowWidget::WindowWidget(App& app, int width, int height)
    : Widget({width, height}), pimpl{new impl{}} {
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

WindowWidget::~WindowWidget() {
    [pimpl->window_controller invalidateAppWindowPointer];
    [pimpl->window_controller release];
}

void WindowWidget::show() {
    [pimpl->window_controller show];
}

void WindowWidget::close() {
    [pimpl->window_controller close];
}

void WindowWidget::redraw() {
    [pimpl->window_controller redraw];
}

int WindowWidget::scale() const {
    return [pimpl->window_controller getScaleFactor];
}

bool WindowWidget::isDarkMode() const {
    return [pimpl->window_controller isDarkMode];
}

void WindowWidget::setTitle(std::string_view title) {
    [pimpl->window_controller setTitle:title];
}

void WindowWidget::setFilePath(std::string_view path) {
    [pimpl->window_controller setFilePath:path];
}

std::optional<std::string> WindowWidget::openFilePicker() const {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    panel.title = @"Choose File";
    panel.prompt = @"Choose";
    panel.canChooseDirectories = false;
    panel.canChooseFiles = true;

    // TODO: Make this a parameter.
    // TODO: Expand tilde paths in a helper function (probably in //base/files/) and implement
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

void WindowWidget::setCursorStyle(CursorStyle style) {
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

void WindowWidget::setAutoRedraw(bool auto_redraw) {
    [pimpl->window_controller setAutoRedraw:auto_redraw];
}

int WindowWidget::framesPerSecond() const {
    return [pimpl->window_controller framesPerSecond];
}

}  // namespace gui
