#pragma once

#include "font/font_rasterizer.h"

#include <d2d1.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <wrl/client.h>

namespace font {

struct FontRasterizer::NativeFontType {
    Microsoft::WRL::ComPtr<IDWriteFont> font;
};

class FontRasterizer::impl {
public:
    Microsoft::WRL::ComPtr<IDWriteFactory4> dwrite_factory;
    Microsoft::WRL::ComPtr<ID2D1Factory> d2d1_factory;
    Microsoft::WRL::ComPtr<IWICImagingFactory2> wic_factory;
    Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> dc_target;

    struct DWriteInfo {
        std::wstring font_name16;
        float em_size;
    };
    std::vector<DWriteInfo> font_id_to_dwrite_info;
    std::wstring locale;
};

}  // namespace font
