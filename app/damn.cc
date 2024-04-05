#include "util/profile_util.h"
#include <glad/glad.h>
#include <glad/glad_wgl.h>
#include <iostream>
#include <shellscalingapi.h>
#include <windows.h>

#define ID_QUIT 0x70

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

class MainWindow : public BaseWindow<MainWindow> {
public:
    MainWindow() {}

    PCWSTR className() const {
        return L"Clock Window Class";
    }
    LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

HDC ghDC;
HGLRC ghRC;

LRESULT MainWindow::handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        ghDC = GetDC(hwnd);

        PIXELFORMATDESCRIPTOR pfd = {
            .nSize = sizeof(PIXELFORMATDESCRIPTOR),
            .nVersion = 1,
            .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            .iPixelType = PFD_TYPE_RGBA,
            .cDepthBits = 24,
            .cStencilBits = 0,
            .cAuxBuffers = 0,
        };

        int pixelformat = ChoosePixelFormat(ghDC, &pfd);
        SetPixelFormat(ghDC, pixelformat, &pfd);

        ghRC = wglCreateContext(ghDC);
        wglMakeCurrent(ghDC, ghRC);

        if (!gladLoadGL() || !gladLoadWGL(ghDC)) {
            std::cerr << "Failed to initialize GLAD\n";
        }

        std::cerr << glGetString(GL_VERSION) << '\n';

        wglSwapIntervalEXT(0);

        std::cerr << wglGetSwapIntervalEXT() << '\n';

        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);

        glClearColor(1.0, 0.0, 0.0, 1.0);

        return 0;
    }
    case WM_PAINT:
    case WM_DISPLAYCHANGE: {
        std::cerr << "WM_PAINT\n";

        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);

        {
            PROFILE_BLOCK("draw");

            glClear(GL_COLOR_BUFFER_BIT);

            SwapBuffers(ghDC);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case ID_QUIT:
            PostQuitMessage(0);
            break;
        }
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// Constants
const WCHAR WINDOW_NAME[] = L"Simple Text";

// TODO: Don't hard code this!
int scale_factor = 2;

INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, INT nCmdShow) {
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    MainWindow win;

    if (!win.create(WINDOW_NAME, WS_OVERLAPPEDWINDOW)) {
        return 0;
    }

    LPCTSTR cursor = IDC_IBEAM;
    HCURSOR hCursor = LoadCursor(NULL, cursor);
    SetCursor(hCursor);

    // ShowWindow(win.window(), nCmdShow);

    // https://stackoverflow.com/a/20624817
    // FIXME: This doesn't animate like ShowWindow().
    // TODO: Replace magic numbers with actual defaults and/or window size restoration.
    WINDOWPLACEMENT placement{
        .length = sizeof(WINDOWPLACEMENT),
        // .showCmd = SW_MAXIMIZE,
        .showCmd = SW_NORMAL,
        .rcNormalPosition = RECT{0, 0, 1000 * scale_factor, 500 * scale_factor},
    };
    SetWindowPlacement(win.window(), &placement);

    // We need to pass `key` as a virtual key in order to combine it with FCONTROL.
    // https://stackoverflow.com/a/53657941
    ACCEL quit_accel{
        .fVirt = FCONTROL | FVIRTKEY,
        .key = LOBYTE(VkKeyScan('q')),
        .cmd = ID_QUIT,
    };

    std::vector<ACCEL> accels = {quit_accel};
    HACCEL hAccel = CreateAcceleratorTable(&accels[0], accels.size());

    // Run the message loop.

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(win.window(), hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return 0;
}
