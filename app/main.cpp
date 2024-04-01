#include <D2d1.h>
#include <assert.h>
#include <atlbase.h>
#include <windows.h>

#include "scene.h"
#include "ui/renderer/rect_renderer.h"
#include "ui/win32/base_window.h"
#include "ui/win32/resource.h"
#include <glad/glad.h>
#include <iostream>
#include <shellscalingapi.h>
#include <vector>

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
    const float y = 200;
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

INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, INT nCmdShow) {
    if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) {
        return 0;
    }

    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    MainWindow win;

    if (!win.Create(WINDOW_NAME, WS_OVERLAPPEDWINDOW)) {
        return 0;
    }

    LPCTSTR cursor = IDC_ARROW;
    HCURSOR hCursor = LoadCursor(NULL, cursor);
    SetCursor(hCursor);

    ShowWindow(win.Window(), nCmdShow);

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
        // if (TranslateAccelerator(win.Window(), hAccel, &msg)) {
        //     continue;
        // }

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

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HWND hwnd = m_hwnd;

    switch (uMsg) {
    case WM_CREATE: {
        if (FAILED(m_scene.Initialize()) || !InitializeTimer()) {
            return -1;
        }

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

        RECT rect = {0};
        GetClientRect(m_hwnd, &rect);

        float scaled_width = rect.right;
        float scaled_height = rect.bottom;

        rect_renderer.setup(scaled_width, scaled_height);

        return 0;
    }

    case WM_DESTROY:
        CloseHandle(m_hTimer);
        m_scene.CleanUp();
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    case WM_DISPLAYCHANGE: {
        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);

        bool use_opengl = false;
        if (use_opengl) {
            RECT rect = {0};
            GetClientRect(m_hwnd, &rect);

            float line_height = 40;
            float scaled_width = rect.right;
            float scaled_height = rect.bottom;
            float scaled_editor_offset_x = 200 * 2;
            float scaled_editor_offset_y = 30 * 2;
            float scaled_status_bar_height = line_height;

            glClear(GL_COLOR_BUFFER_BIT);

            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
            rect_renderer.resize(scaled_width, scaled_height);
            rect_renderer.draw(0, 0, 0, 0, line_height, 100, 500, scaled_editor_offset_x,
                               scaled_editor_offset_y, scaled_status_bar_height);

            SwapBuffers(ghDC);
        } else {
            m_scene.Render(m_hwnd);
        }

        EndPaint(m_hwnd, &ps);
    }
        return 0;

    case WM_SIZE: {
        int x = (int)(short)LOWORD(lParam);
        int y = (int)(short)HIWORD(lParam);
        m_scene.Resize(x, y);
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
        return 0;

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
