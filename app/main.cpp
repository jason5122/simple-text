#define WIN32_LEAN_AND_MEAN
// https://stackoverflow.com/a/13420838/14698275
#define NOMINMAX
#include <windows.h>

#include <D2d1.h>
#include <assert.h>
#include <atlbase.h>

#include "base/buffer.h"
#include "base/syntax_highlighter.h"
#include "font/rasterizer.h"
#include "scene.h"
#include "ui/renderer/image_renderer.h"
#include "ui/renderer/rect_renderer.h"
#include "ui/renderer/text_renderer.h"
#include "ui/win32/base_window.h"
#include "ui/win32/resource.h"
#include "util/profile_util.h"
#include <dwrite.h>
#include <glad/glad.h>
#include <iostream>
#include <shellscalingapi.h>
#include <vector>
#include <windowsx.h>
#include <winuser.h>

class Scene : public GraphicsScene {
    CComPtr<ID2D1SolidColorBrush> m_pFill;
    CComPtr<ID2D1SolidColorBrush> m_pStroke;

    D2D1_ELLIPSE m_ellipse;
    D2D_POINT_2F m_Ticks[24];

    HRESULT CreateDeviceIndependentResources() {
        return S_OK;
    }
    void DiscardDeviceIndependentResources() {}
    HRESULT CreateDeviceDependentResources();
    void DiscardDeviceDependentResources();
    void CalculateLayout();
    void RenderScene();

    void DrawClockHand(float fHandLength, float fAngle, float fStrokeWidth);
};

HRESULT Scene::CreateDeviceDependentResources() {
    HRESULT hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 0),
                                                        D2D1::BrushProperties(), &m_pFill);

    if (SUCCEEDED(hr)) {
        hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0), D2D1::BrushProperties(),
                                                    &m_pStroke);
    }
    return hr;
}

void Scene::DrawClockHand(float fHandLength, float fAngle, float fStrokeWidth) {
    m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(fAngle, m_ellipse.point));

    // endPoint defines one end of the hand.
    D2D_POINT_2F endPoint =
        D2D1::Point2F(m_ellipse.point.x, m_ellipse.point.y - (m_ellipse.radiusY * fHandLength));

    // Draw a line from the center of the ellipse to endPoint.
    m_pRenderTarget->DrawLine(m_ellipse.point, endPoint, m_pStroke, fStrokeWidth);
}

void Scene::RenderScene() {
    m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));

    m_pRenderTarget->FillEllipse(m_ellipse, m_pFill);
    m_pRenderTarget->DrawEllipse(m_ellipse, m_pStroke);

    // Draw tick marks
    for (DWORD i = 0; i < 12; i++) {
        m_pRenderTarget->DrawLine(m_Ticks[i * 2], m_Ticks[i * 2 + 1], m_pStroke, 2.0f);
    }

    // Draw hands
    SYSTEMTIME time;
    GetLocalTime(&time);

    // 60 minutes = 30 degrees, 1 minute = 0.5 degree
    const float fHourAngle = (360.0f / 12) * (time.wHour) + (time.wMinute * 0.5f);
    const float fMinuteAngle = (360.0f / 60) * (time.wMinute);

    const float fSecondAngle =
        (360.0f / 60) * (time.wSecond) + (360.0f / 60000) * (time.wMilliseconds);

    DrawClockHand(0.6f, fHourAngle, 6);
    DrawClockHand(0.85f, fMinuteAngle, 4);
    DrawClockHand(0.85f, fSecondAngle, 1);

    // Restore the identity transformation.
    m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
}

void Scene::CalculateLayout() {
    D2D1_SIZE_F fSize = m_pRenderTarget->GetSize();

    const float x = 200;
    const float y = fSize.height - 200;
    const float radius = 200;

    m_ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius);

    // Calculate tick marks.

    D2D_POINT_2F pt1 =
        D2D1::Point2F(m_ellipse.point.x, m_ellipse.point.y - (m_ellipse.radiusY * 0.9f));

    D2D_POINT_2F pt2 = D2D1::Point2F(m_ellipse.point.x, m_ellipse.point.y - m_ellipse.radiusY);

    for (DWORD i = 0; i < 12; i++) {
        D2D1::Matrix3x2F mat = D2D1::Matrix3x2F::Rotation((360.0f / 12) * i, m_ellipse.point);

        m_Ticks[i * 2] = mat.TransformPoint(pt1);
        m_Ticks[i * 2 + 1] = mat.TransformPoint(pt2);
    }
}

void Scene::DiscardDeviceDependentResources() {
    m_pFill.Release();
    m_pStroke.Release();
}

class MainWindow : public BaseWindow<MainWindow> {
    HANDLE m_hTimer;
    Scene m_scene;
    IDWriteFactory* dwrite_factory;

    BOOL InitializeTimer();

public:
    MainWindow() : m_hTimer(NULL) {}

    void WaitTimer();

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
const WCHAR WINDOW_NAME[] = L"Analog Clock";

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
    while (msg.message != WM_QUIT) {
        // TODO: Investigate if accelerator tables slow down the drawing loop.
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) &&
            !TranslateAccelerator(win.Window(), hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        win.WaitTimer();
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
        // if (FAILED(m_scene.Initialize()) || !InitializeTimer()) {
        //     return -1;
        // }

        m_scene.Initialize();

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

        fs::path file_path = ResourcePath() / "sample_files/worst_case.json";

        RECT rect = {0};
        GetClientRect(m_hwnd, &rect);

        float scaled_width = rect.right;
        float scaled_height = rect.bottom;

        main_font_rasterizer.setup(0, "Source Code Pro", 11 * scale_factor);
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
        CloseHandle(m_hTimer);
        m_scene.CleanUp();
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    case WM_DISPLAYCHANGE: {
        // Sleep(25);  // Add artificial lag for testing.

        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);

        bool use_opengl = true;
        if (use_opengl) {
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
                float scaled_status_bar_height = line_height;

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
            }

            SwapBuffers(ghDC);
        } else {
            {
                PROFILE_BLOCK("D2D1 draw");
                m_scene.Render(m_hwnd);
            }
        }

        EndPaint(m_hwnd, &ps);
        return 0;
    }

    case WM_SIZE: {
        int x = (int)(short)LOWORD(lParam);
        int y = (int)(short)HIWORD(lParam);
        rect_renderer.resize(x, y);
        image_renderer.resize(x, y);
        m_scene.Resize(x, y);

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
        dy *= 40;

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

BOOL MainWindow::InitializeTimer() {
    m_hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
    if (m_hTimer == NULL) {
        return FALSE;
    }

    LARGE_INTEGER li = {0};

    if (!SetWaitableTimer(m_hTimer, &li, (1000 / 60), NULL, NULL, FALSE)) {
        CloseHandle(m_hTimer);
        m_hTimer = NULL;
        return FALSE;
    }

    return TRUE;
}

void MainWindow::WaitTimer() {
    // Wait until the timer expires or any message is posted.
    if (MsgWaitForMultipleObjects(1, &m_hTimer, FALSE, INFINITE, QS_ALLINPUT) == WAIT_OBJECT_0) {
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
}
