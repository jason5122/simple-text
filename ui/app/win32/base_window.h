#pragma once

#include <windows.h>

template <class DERIVED_TYPE> class BaseWindow {
public:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        DERIVED_TYPE* pThis = NULL;

        if (uMsg == WM_NCCREATE) {
            CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
            pThis = (DERIVED_TYPE*)pCreate->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);

            pThis->hwnd = hwnd;
        } else {
            pThis = (DERIVED_TYPE*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }
        if (pThis) {
            return pThis->handleMessage(uMsg, wParam, lParam);
        } else {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

    BaseWindow() : hwnd(NULL) {}

    BOOL create(PCWSTR lpWindowName, DWORD dwStyle, DWORD dwExStyle = 0, int x = CW_USEDEFAULT,
                int y = CW_USEDEFAULT, int nWidth = CW_USEDEFAULT, int nHeight = CW_USEDEFAULT,
                HWND hWndParent = 0, HMENU hMenu = 0) {
        WNDCLASS wc{
            .lpfnWndProc = DERIVED_TYPE::WindowProc,
            .hInstance = GetModuleHandle(NULL),
            // TODO: Change this color based on the editor background color.
            .hbrBackground = CreateSolidBrush(RGB(253, 253, 253)),
            .lpszClassName = className(),
        };

        RegisterClass(&wc);

        hwnd = CreateWindowEx(dwExStyle, className(), lpWindowName, dwStyle, x, y, nWidth, nHeight,
                              hWndParent, hMenu, GetModuleHandle(NULL), this);

        return (hwnd ? TRUE : FALSE);
    }

    HWND window() const {
        return hwnd;
    }

protected:
    virtual PCWSTR className() const = 0;
    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

    HWND hwnd;
};
