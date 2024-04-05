#include "ui/app/app.h"
#include <glad/glad.h>
#include <vector>
#include <windowsx.h>

#define WIN32_LEAN_AND_MEAN
// https://stackoverflow.com/a/13420838/14698275
#define NOMINMAX
#include <windows.h>

#define ID_QUIT 0x70
#define ID_CLOSE_WINDOW 0x71

HDC ghDC;
HGLRC ghRC;

BOOL bSetupPixelFormat(HDC hdc) {
    PIXELFORMATDESCRIPTOR pfd = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cDepthBits = 24,
        .cStencilBits = 0,
        .cAuxBuffers = 0,
    };

    int pixelformat = ChoosePixelFormat(hdc, &pfd);
    if (!pixelformat) {
        return false;
    }
    return SetPixelFormat(hdc, pixelformat, &pfd);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        ghDC = GetDC(hwnd);
        if (!bSetupPixelFormat(ghDC)) {
            PostQuitMessage(0);
        }
        ghRC = wglCreateContext(ghDC);
        wglMakeCurrent(ghDC, ghRC);

        if (!gladLoadGL()) {
            std::cerr << "Failed to initialize GLAD\n";
        }

        std::cerr << glGetString(GL_VERSION) << '\n';

        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);

        glClearColor(1.0, 0.0, 0.0, 1.0);

        RECT rect = {0};
        GetClientRect(hwnd, &rect);

        float scaled_width = rect.right;
        float scaled_height = rect.bottom;

        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    case WM_DISPLAYCHANGE: {
        std::cerr << "WM_PAINT\n";

        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);

        glClear(GL_COLOR_BUFFER_BIT);

        SwapBuffers(ghDC);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_SIZE: {
        int new_width = (int)(short)LOWORD(lParam);
        int new_height = (int)(short)HIWORD(lParam);

        std::cerr << "WM_SIZE\n";

        // FIXME: Window sometimes does not redraw correctly when maximizing/un-maximizing.
        InvalidateRect(hwnd, NULL, FALSE);
        // RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

        return 0;
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case ID_QUIT:
            PostQuitMessage(0);
            break;
        case ID_CLOSE_WINDOW:
            std::cerr << "ID_CLOSE_WINDOW\n";
            PostQuitMessage(0);
            break;
        }
        return 0;
    }

    case WM_MOUSEWHEEL: {
        float dy = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;

        std::cerr << "WM_MOUSEWHEEL\n";

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_MOUSEHWHEEL: {
        float dx = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam));

        std::cerr << "WM_MOUSEHWHEEL\n";

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        SetCapture(hwnd);

        int mouse_x = GET_X_LPARAM(lParam);
        int mouse_y = GET_Y_LPARAM(lParam);

        std::cerr << "WM_LBUTTONDOWN\n";

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_LBUTTONUP: {
        ReleaseCapture();
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (wParam == MK_LBUTTON) {
            int mouse_x = GET_X_LPARAM(lParam);
            int mouse_y = GET_Y_LPARAM(lParam);

            std::cerr << "WM_MOUSEMOVE\n";

            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

const WCHAR CLASS_NAME[] = L"Sample Window Class";
const WCHAR WINDOW_NAME[] = L"Simple Text";

// TODO: Don't hard code this!
int scale_factor = 2;

class App::impl {
public:
    HWND hwnd;
};

App::App() : pimpl{new impl{}} {
    WNDCLASS wc{
        .lpfnWndProc = WindowProc,
        .hInstance = GetModuleHandle(nullptr),
        // TODO: Change this color based on the editor background color.
        .hbrBackground = CreateSolidBrush(RGB(253, 253, 253)),
        // TODO: Change this.
        .lpszClassName = CLASS_NAME,
    };
    RegisterClass(&wc);

    LPCTSTR cursor = IDC_IBEAM;
    HCURSOR hCursor = LoadCursor(NULL, cursor);
    SetCursor(hCursor);
}

void App::run() {
    this->onActivate();

    // We need to pass `key` as a virtual key in order to combine it with FCONTROL.
    // https://stackoverflow.com/a/53657941
    ACCEL quit_accel{
        .fVirt = FCONTROL | FVIRTKEY,
        .key = LOBYTE(VkKeyScan('q')),
        .cmd = ID_QUIT,
    };
    ACCEL close_accel{
        .fVirt = FCONTROL | FVIRTKEY,
        .key = LOBYTE(VkKeyScan('w')),
        .cmd = ID_CLOSE_WINDOW,
    };

    std::vector<ACCEL> accels = {quit_accel, close_accel};
    HACCEL hAccel = CreateAcceleratorTable(&accels[0], accels.size());

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(pimpl->hwnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

void App::createNewWindow() {
    pimpl->hwnd = CreateWindowEx(0, CLASS_NAME, WINDOW_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr,
                                 GetModuleHandle(nullptr), nullptr);

    // ShowWindow(win.Window(), nCmdShow);

    // https://stackoverflow.com/a/20624817
    // FIXME: This doesn't animate like ShowWindow().
    // TODO: Replace magic numbers with actual defaults and/or window size restoration.
    WINDOWPLACEMENT placement{
        .length = sizeof(WINDOWPLACEMENT),
        .showCmd = SW_SHOWMAXIMIZED,
        .rcNormalPosition = RECT{0, 0, 1000 * scale_factor, 500 * scale_factor},
    };
    SetWindowPlacement(pimpl->hwnd, &placement);
}

App::~App() {}
