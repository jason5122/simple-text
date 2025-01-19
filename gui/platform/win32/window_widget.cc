#include "gui/platform/window_widget.h"

#include "gui/platform/win32/impl_win.h"

#include <fmt/base.h>

namespace gui {

WindowWidget::WindowWidget(App& app, int width, int height)
    : pimpl{new impl{*this, app.pimpl->dummy_context}} {}

WindowWidget::~WindowWidget() {}

void WindowWidget::show() {
    pimpl->win32_window.create(L"Simple Text", WS_OVERLAPPEDWINDOW, pimpl->wid++);

    // TODO: Sync this with requested width/height.
    int width = 1200;
    int height = 600;

    // FIXME: This doesn't animate like ShowWindow().
    WINDOWPLACEMENT placement{
        .length = sizeof(WINDOWPLACEMENT),
        // .showCmd = SW_MAXIMIZE,
        .showCmd = SW_NORMAL,
        .rcNormalPosition = RECT{0, 0, width * 2, height * 2},
    };
    SetWindowPlacement(pimpl->win32_window.get_hwnd(), &placement);
}

void WindowWidget::close() {
    pimpl->win32_window.destroy();
}

void WindowWidget::redraw() {
    pimpl->win32_window.redraw();
}

int WindowWidget::scale() const {
    return pimpl->win32_window.scale();
}

bool WindowWidget::isDarkMode() const {
    return false;
}

void WindowWidget::setTitle(std::string_view title) {
    pimpl->win32_window.set_title(title);
}

// TODO: Implement this.
void WindowWidget::setFilePath(std::string_view path) {
    ;
}

// TODO: Implement this.
std::optional<std::string> WindowWidget::openFilePicker() const {
    return {};
}

// TODO: Implement this.
void WindowWidget::setCursorStyle(CursorStyle style) {
    if (current_style == style) return;
    current_style = style;

    if (style == CursorStyle::kArrow) {
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
    } else if (style == CursorStyle::kIBeam) {
        SetCursor(LoadCursor(nullptr, IDC_IBEAM));
    } else if (style == CursorStyle::kResizeLeftRight) {
        SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
    } else if (style == CursorStyle::kResizeUpDown) {
        SetCursor(LoadCursor(nullptr, IDC_SIZENS));
    }
}

// TODO: Implement this.
void WindowWidget::setAutoRedraw(bool auto_redraw) {
    ;
}

// TODO: Implement this.
int WindowWidget::framesPerSecond() const {
    return 60;
}

}  // namespace gui
