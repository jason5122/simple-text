#include <D2d1.h>
#include <assert.h>
#include <atlbase.h>
#include <windows.h>

#include "scene.h"
#include "ui/renderer/image_renderer.h"
#include "ui/renderer/rect_renderer.h"
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

    ShowWindow(win.Window(), nCmdShow);

    // https://stackoverflow.com/a/20624817
    // FIXME: This doesn't animate like ShowWindow().
    // TODO: Replace magic numbers with actual defaults and/or window size restoration.
    // WINDOWPLACEMENT placement{
    //     .length = sizeof(WINDOWPLACEMENT),
    //     .showCmd = SW_SHOWMAXIMIZED,
    //     .rcNormalPosition = RECT{0, 0, 1000 * scale_factor, 500 * scale_factor},
    // };
    // SetWindowPlacement(win.Window(), &placement);

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
double scroll_x = 0;
double scroll_y = 0;

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

        RECT rect = {0};
        GetClientRect(m_hwnd, &rect);

        float scaled_width = rect.right;
        float scaled_height = rect.bottom;

        rect_renderer.setup(scaled_width, scaled_height);
        image_renderer.setup(scaled_width, scaled_height);

        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                            reinterpret_cast<IUnknown**>(&dwrite_factory));

        IDWriteFontCollection* font_collection;
        dwrite_factory->GetSystemFontCollection(&font_collection);

        // https://stackoverflow.com/q/40365439/14698275
        UINT32 index;
        BOOL exists;
        font_collection->FindFamilyName(L"Source Code Pro", &index, &exists);

        if (exists) {
            std::cerr << "Font family found!\n";

            IDWriteFontFamily* font_family;
            font_collection->GetFontFamily(index, &font_family);

            // IDWriteLocalizedStrings* family_names;
            // font_family->GetFamilyNames(&family_names);

            // UINT32 length = 0;
            // family_names->GetStringLength(0, &length);

            // wchar_t* name = new (std::nothrow) wchar_t[length + 1];
            // family_names->GetString(0, name, length + 1);
            // fwprintf(stderr, L"%s\n", name);

            IDWriteFont* font;
            font_family->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_REGULAR,
                                              DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                              &font);
            IDWriteFontFace* font_face;
            font->CreateFontFace(&font_face);

            UINT32 codepoint = 'L';
            UINT16* glyph_indices = new UINT16[1];
            font_face->GetGlyphIndices(&codepoint, 1, glyph_indices);

            std::cerr << glyph_indices[0] << '\n';

            // TODO: Verify that this is correct.
            FLOAT font_size = 32.0;
            FLOAT em_size = font_size * 96 / 72;

            FLOAT glyph_advances = 0;
            DWRITE_GLYPH_OFFSET offset = {0};
            DWRITE_GLYPH_RUN glyph_run{
                .fontFace = font_face,
                .fontEmSize = em_size,
                .glyphCount = 1,
                .glyphIndices = glyph_indices,
                .glyphAdvances = &glyph_advances,
                .glyphOffsets = &offset,
                .isSideways = 0,
                .bidiLevel = 0,
            };

            IDWriteRenderingParams* rendering_params;
            dwrite_factory->CreateRenderingParams(&rendering_params);

            DWRITE_RENDERING_MODE rendering_mode;
            font_face->GetRecommendedRenderingMode(em_size, 1.0, DWRITE_MEASURING_MODE_NATURAL,
                                                   rendering_params, &rendering_mode);

            IDWriteGlyphRunAnalysis* glyph_run_analysis;
            dwrite_factory->CreateGlyphRunAnalysis(&glyph_run, 1.0, nullptr, rendering_mode,
                                                   DWRITE_MEASURING_MODE_NATURAL, 0.0, 0.0,
                                                   &glyph_run_analysis);

            RECT texture_bounds;
            glyph_run_analysis->GetAlphaTextureBounds(DWRITE_TEXTURE_CLEARTYPE_3x1,
                                                      &texture_bounds);

            LONG pixel_width = texture_bounds.right - texture_bounds.left;
            LONG pixel_height = texture_bounds.bottom - texture_bounds.top;
            UINT32 pixels = pixel_width * pixel_height;
            BYTE* alpha_values = new BYTE[pixel_width * pixel_height];
            glyph_run_analysis->CreateAlphaTexture(DWRITE_TEXTURE_CLEARTYPE_3x1, &texture_bounds,
                                                   alpha_values, pixel_width * pixel_height);

            std::cerr << pixel_width << 'x' << pixel_height << '\n';
            // for (size_t r = 0; r < pixel_height; r++) {
            //     for (size_t c = 0; c < pixel_width; c++) {
            //         std::cerr << +alpha_values[r * pixel_width + c] << ' ';
            //     }
            // }
            // std::cerr << '\n';
        }

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

                float line_height = 40;
                float scaled_width = rect.right;
                float scaled_height = rect.bottom;
                double scaled_scroll_x = scroll_x * scale_factor;
                double scaled_scroll_y = scroll_y * scale_factor;
                float scaled_editor_offset_x = 200 * scale_factor;
                float scaled_editor_offset_y = 30 * scale_factor;
                float scaled_status_bar_height = line_height;

                glClear(GL_COLOR_BUFFER_BIT);

                glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
                rect_renderer.draw(scaled_scroll_x, scaled_scroll_y, 0, 0, line_height, 100, 500,
                                   scaled_editor_offset_x, scaled_editor_offset_y,
                                   scaled_status_bar_height);

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
        float dy = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam));
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
        //     InvalidateRect(m_hwnd, NULL, FALSE);
        //     return 0;
        // }

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
