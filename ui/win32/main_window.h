#pragma once

#include "ui/win32/base_window.h"

class MainWindow : public BaseWindow<MainWindow> {
public:
    PCWSTR ClassName() const {
        return L"Sample Window Class";
    }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        EndPaint(m_hwnd, &ps);
    }
        return 0;

    default:
        return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
    }
    return TRUE;
}
