#include "gui/cocoa/pimpl_mac.h"
#include "gui/window.h"

Window2::Window2(App& app) : pimpl{new impl{}}, app(app) {
    // NSRect frame = NSMakeRect(500, 0, width, height);
    NSRect frame = NSScreen.mainScreen.visibleFrame;

    frame.origin.y = 300;
    frame.size.height -= 300;

    std::cerr << "scale factor: " << NSScreen.mainScreen.backingScaleFactor << '\n';

    DisplayGL* displaygl = app.pimpl->displaygl.get();
    pimpl->window_controller =
        [[WindowController alloc] initWithFrame:frame appWindow:this displayGl:displaygl];

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

Window2::~Window2() {
    [pimpl->window_controller release];
}

void Window2::show() {
    [pimpl->window_controller show];
}

void Window2::close() {
    [pimpl->window_controller close];
}

void Window2::redraw() {
    [pimpl->window_controller redraw];
}

int Window2::width() {
    return [pimpl->window_controller getWidth];
}

int Window2::height() {
    return [pimpl->window_controller getHeight];
}

int Window2::scaleFactor() {
    return [pimpl->window_controller getScaleFactor];
}

bool Window2::isDarkMode() {
    return [pimpl->window_controller isDarkMode];
}
