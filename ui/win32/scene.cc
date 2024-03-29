#include "scene.h"

HRESULT GraphicsScene::CreateGraphicsResources(HWND hwnd) {
    HRESULT hr = S_OK;
    if (m_pRenderTarget == NULL) {
        RECT rc;
        GetClientRect(hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        hr = m_pFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
                                                D2D1::HwndRenderTargetProperties(hwnd, size),
                                                &m_pRenderTarget);

        if (SUCCEEDED(hr)) {
            hr = CreateDeviceDependentResources();
        }
        if (SUCCEEDED(hr)) {
            CalculateLayout();
        }
    }
    return hr;
}

template <class T> T GraphicsScene::PixelToDipX(T pixels) const {
    return static_cast<T>(pixels / m_fScaleX);
}

template <class T> T GraphicsScene::PixelToDipY(T pixels) const {
    return static_cast<T>(pixels / m_fScaleY);
}

HRESULT GraphicsScene::Initialize() {
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pFactory);

    if (SUCCEEDED(hr)) {
        CreateDeviceIndependentResources();
    }
    return hr;
}

void GraphicsScene::Render(HWND hwnd) {
    HRESULT hr = CreateGraphicsResources(hwnd);
    if (FAILED(hr)) {
        return;
    }

    assert(m_pRenderTarget != NULL);

    m_pRenderTarget->BeginDraw();

    RenderScene();

    hr = m_pRenderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        DiscardDeviceDependentResources();
        m_pRenderTarget.Release();
    }
}

HRESULT GraphicsScene::Resize(int x, int y) {
    HRESULT hr = S_OK;
    if (m_pRenderTarget) {
        hr = m_pRenderTarget->Resize(D2D1::SizeU(x, y));
        if (SUCCEEDED(hr)) {
            CalculateLayout();
        }
    }
    return hr;
}

void GraphicsScene::CleanUp() {
    DiscardDeviceDependentResources();
    DiscardDeviceIndependentResources();
}
