#pragma once

#include <D2d1.h>
#include <assert.h>
#include <atlbase.h>

class GraphicsScene {
protected:
    // D2D Resources
    CComPtr<ID2D1Factory> m_pFactory;
    CComPtr<ID2D1HwndRenderTarget> m_pRenderTarget;

    float m_fScaleX;
    float m_fScaleY;

protected:
    // Derived class must implement these methods.
    virtual HRESULT CreateDeviceIndependentResources() = 0;
    virtual void DiscardDeviceIndependentResources() = 0;
    virtual HRESULT CreateDeviceDependentResources() = 0;
    virtual void DiscardDeviceDependentResources() = 0;
    virtual void CalculateLayout() = 0;
    virtual void RenderScene() = 0;

protected:
    HRESULT CreateGraphicsResources(HWND hwnd);

    template <class T> T PixelToDipX(T pixels) const;
    template <class T> T PixelToDipY(T pixels) const;

public:
    GraphicsScene() : m_fScaleX(1.0f), m_fScaleY(1.0f) {}
    virtual ~GraphicsScene() {}

    HRESULT Initialize();

    void Render(HWND hwnd);

    HRESULT Resize(int x, int y);

    void CleanUp();
};
