#define WIN32_LEAN_AND_MEAN
// https://stackoverflow.com/a/13420838/14698275
#define NOMINMAX
#include <windows.h>

#include "base/buffer.h"
#include "base/syntax_highlighter.h"
#include "font/rasterizer.h"
#include "resource.h"
#include "ui/renderer/image_renderer.h"
#include "ui/renderer/rect_renderer.h"
#include "ui/renderer/text_renderer.h"
#include "util/profile_util.h"
#include <atlbase.h>
#include <dwrite.h>
#include <glad/glad.h>
#include <iostream>
#include <shellscalingapi.h>
#include <vector>
#include <windowsx.h>
#include <winuser.h>

template <class DERIVED_TYPE> class BaseWindow {
public:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        DERIVED_TYPE* pThis = NULL;

        if (uMsg == WM_NCCREATE) {
            CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
            pThis = (DERIVED_TYPE*)pCreate->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);

            pThis->m_hwnd = hwnd;
        } else {
            pThis = (DERIVED_TYPE*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }
        if (pThis) {
            return pThis->HandleMessage(uMsg, wParam, lParam);
        } else {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

    BaseWindow() : m_hwnd(NULL) {}

    BOOL Create(PCWSTR lpWindowName, DWORD dwStyle, DWORD dwExStyle = 0, int x = CW_USEDEFAULT,
                int y = CW_USEDEFAULT, int nWidth = CW_USEDEFAULT, int nHeight = CW_USEDEFAULT,
                HWND hWndParent = 0, HMENU hMenu = 0) {
        WNDCLASS wc{
            .lpfnWndProc = DERIVED_TYPE::WindowProc,
            .hInstance = GetModuleHandle(NULL),
            // TODO: Change this color based on the editor background color.
            .hbrBackground = CreateSolidBrush(RGB(253, 253, 253)),
            .lpszClassName = ClassName(),
        };

        RegisterClass(&wc);

        m_hwnd = CreateWindowEx(dwExStyle, ClassName(), lpWindowName, dwStyle, x, y, nWidth,
                                nHeight, hWndParent, hMenu, GetModuleHandle(NULL), this);

        return (m_hwnd ? TRUE : FALSE);
    }

    HWND Window() const {
        return m_hwnd;
    }

protected:
    virtual PCWSTR ClassName() const = 0;
    virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

    HWND m_hwnd;
};

class MainWindow : public BaseWindow<MainWindow> {
    IDWriteFactory* dwrite_factory;

public:
    MainWindow() {}

    PCWSTR ClassName() const {
        return L"Clock Window Class";
    }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

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

// Constants
const WCHAR WINDOW_NAME[] = L"Simple Text";

// TODO: Don't hard code this!
int scale_factor = 2;

INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, INT nCmdShow) {
    if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) {
        return 0;
    }

    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    MainWindow win;

    if (!win.Create(WINDOW_NAME, WS_OVERLAPPEDWINDOW)) {
        return 0;
    }

    LPCTSTR cursor = IDC_IBEAM;
    HCURSOR hCursor = LoadCursor(NULL, cursor);
    SetCursor(hCursor);

    // ShowWindow(win.Window(), nCmdShow);

    // https://stackoverflow.com/a/20624817
    // FIXME: This doesn't animate like ShowWindow().
    // TODO: Replace magic numbers with actual defaults and/or window size restoration.
    WINDOWPLACEMENT placement{
        .length = sizeof(WINDOWPLACEMENT),
        .showCmd = SW_SHOWMAXIMIZED,
        .rcNormalPosition = RECT{0, 0, 1000 * scale_factor, 500 * scale_factor},
    };
    SetWindowPlacement(win.Window(), &placement);

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
        if (!TranslateAccelerator(win.Window(), hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    CoUninitialize();
    return 0;
}

HDC ghDC;
HGLRC ghRC;

RectRenderer rect_renderer;
ImageRenderer image_renderer;
TextRenderer text_renderer;
FontRasterizer main_font_rasterizer;
FontRasterizer ui_font_rasterizer;
Buffer buffer;
SyntaxHighlighter highlighter;

double cursor_start_x = 0;
double cursor_start_y = 0;
double scroll_x = 0;
double scroll_y = 0;
float editor_offset_x = 200;
float editor_offset_y = 30;

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HWND hwnd = m_hwnd;

    switch (uMsg) {
    case WM_CREATE: {
        ghDC = GetDC(m_hwnd);
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

        glClearColor(253 / 255.0, 253 / 255.0, 253 / 255.0, 1.0);

        // fs::path file_path = ResourcePath() / "sample_files/sort.scm";
        // fs::path file_path = ResourcePath() / "sample_files/worst_case.json";
        fs::path file_path = ResourcePath() / "sample_files/emojis.txt";

        RECT rect = {0};
        GetClientRect(m_hwnd, &rect);

        float scaled_width = rect.right;
        float scaled_height = rect.bottom;

        main_font_rasterizer.setup(0, "Segoe UI", 11 * scale_factor);
        // main_font_rasterizer.setup(0, "Source Code Pro", 11 * scale_factor);
        ui_font_rasterizer.setup(1, "Segoe UI", 8 * scale_factor);
        text_renderer.setup(scaled_width, scaled_height, main_font_rasterizer);
        rect_renderer.setup(scaled_width, scaled_height);
        image_renderer.setup(scaled_width, scaled_height);
        highlighter.setLanguage("source.json");

        buffer.setContents(ReadFile(file_path));

        TSInput input = {&buffer, Buffer::read, TSInputEncodingUTF8};
        highlighter.parse(input);

        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    case WM_DISPLAYCHANGE: {
        // Sleep(25);  // Add artificial lag for testing.

        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);

        {
            PROFILE_BLOCK("OpenGL draw");
            RECT rect = {0};
            GetClientRect(m_hwnd, &rect);

            float line_height = main_font_rasterizer.line_height;
            float scaled_width = rect.right;
            float scaled_height = rect.bottom;
            double scaled_scroll_x = scroll_x * scale_factor;
            double scaled_scroll_y = scroll_y * scale_factor;
            float scaled_editor_offset_x = editor_offset_x * scale_factor;
            float scaled_editor_offset_y = editor_offset_y * scale_factor;
            float scaled_status_bar_height = ui_font_rasterizer.line_height;

            glClear(GL_COLOR_BUFFER_BIT);

            glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
            text_renderer.resize(scaled_width, scaled_height);
            text_renderer.renderText(scaled_scroll_x, scaled_scroll_y, buffer, highlighter,
                                     scaled_editor_offset_x, scaled_editor_offset_y,
                                     main_font_rasterizer, scaled_status_bar_height);

            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
            rect_renderer.draw(scaled_scroll_x, scaled_scroll_y, text_renderer.cursor_end_x,
                               text_renderer.cursor_end_line, line_height, buffer.lineCount(),
                               text_renderer.longest_line_x, scaled_editor_offset_x,
                               scaled_editor_offset_y, scaled_status_bar_height);

            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
            image_renderer.draw(scaled_scroll_x, scaled_scroll_y, scaled_editor_offset_x,
                                scaled_editor_offset_y);

            // glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
            // text_renderer.renderUiText(main_font_rasterizer, ui_font_rasterizer);
        }

        SwapBuffers(ghDC);

        EndPaint(m_hwnd, &ps);
        return 0;
    }

    case WM_SIZE: {
        int x = (int)(short)LOWORD(lParam);
        int y = (int)(short)HIWORD(lParam);
        rect_renderer.resize(x, y);
        image_renderer.resize(x, y);

        // FIXME: Window sometimes does not redraw correctly when maximizing/un-maximizing.
        InvalidateRect(m_hwnd, NULL, FALSE);
        // RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

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

    case WM_MOUSEWHEEL: {
        float dy = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
        // TODO: Replace this magic number.
        dy *= 50;

        scroll_y -= dy;
        if (scroll_y < 0) {
            scroll_y = 0;
        }
        InvalidateRect(m_hwnd, NULL, FALSE);
        return 0;
    }

        // TODO: Temporarily disable while debugging.
        // case WM_MOUSEHWHEEL: {
        //     float dx = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam));
        //     scroll_x += dx;
        //     if (scroll_x < 0) {
        //         scroll_x = 0;
        //     }
        //     InvalidateRect(m_hwnd, NULL, FALSE);
        //     return 0;
        // }

    case WM_LBUTTONDOWN: {
        SetCapture(m_hwnd);

        int mouse_x = GET_X_LPARAM(lParam);
        int mouse_y = GET_Y_LPARAM(lParam);

        mouse_x -= editor_offset_x * scale_factor;
        mouse_y -= editor_offset_y * scale_factor;

        mouse_x += scroll_x * scale_factor;
        mouse_y += scroll_y * scale_factor;

        cursor_start_x = mouse_x;
        cursor_start_y = mouse_y;

        text_renderer.setCursorPositions(buffer, cursor_start_x, cursor_start_y, mouse_x, mouse_y,
                                         main_font_rasterizer);

        InvalidateRect(m_hwnd, NULL, FALSE);
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

            mouse_x -= editor_offset_x * scale_factor;
            mouse_y -= editor_offset_y * scale_factor;

            mouse_x += scroll_x * scale_factor;
            mouse_y += scroll_y * scale_factor;

            text_renderer.setCursorPositions(buffer, cursor_start_x, cursor_start_y, mouse_x,
                                             mouse_y, main_font_rasterizer);

            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}
