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
    Microsoft::WRL::ComPtr<ID2D1Factory> d2d_factory;
    Microsoft::WRL::ComPtr<IWICImagingFactory2> wic_factory;

    void drawColorRun(ID2D1RenderTarget* target,
                      Microsoft::WRL::ComPtr<IDWriteColorGlyphRunEnumerator1> color_run_enumerator,
                      UINT origin_y);

    struct DWriteInfo {
        std::wstring font_name_utf16;
        FLOAT em_size;
    };
    std::vector<DWriteInfo> font_id_to_dwrite_info;
    const DWriteInfo& getDWriteInfo(size_t font_id);

    std::wstring getPostScriptName(Microsoft::WRL::ComPtr<IDWriteFont> font);
};

}  // namespace font
