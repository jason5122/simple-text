// https://stackoverflow.com/questions/22744262/cant-call-stdmax-because-minwindef-h-defines-max
#define NOMINMAX

#include "base/windows/unicode.h"
#include "font/directwrite/font_fallback_renderer.h"
#include "font/font_rasterizer.h"
#include <combaseapi.h>
#include <comdef.h>
#include <cwchar>
#include <d2d1.h>
#include <iostream>
#include <unknwnbase.h>
#include <vector>
#include <wincodec.h>
#include <winerror.h>
#include <wingdi.h>
#include <winnt.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace font {

class FontRasterizer::impl {
public:
    ComPtr<IDWriteFactory4> dwrite_factory;
    ComPtr<ID2D1Factory> d2d_factory;
    ComPtr<IWICImagingFactory2> wic_factory;

    ComPtr<IDWriteFontFace> font_face;
    std::wstring font_name_utf16;
    FLOAT em_size;

    void drawColorRun(ID2D1RenderTarget* target,
                      ComPtr<IDWriteColorGlyphRunEnumerator1> color_run_enumerator,
                      UINT origin_y);
};

FontRasterizer::FontRasterizer(const std::string& font_name_utf8, int font_size)
    : pimpl{new impl{}} {
    pimpl->font_name_utf16 = base::windows::ConvertToUTF16(font_name_utf8);

    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory4),
                        reinterpret_cast<IUnknown**>(pimpl->dwrite_factory.GetAddressOf()));
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, pimpl->d2d_factory.GetAddressOf());

    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pimpl->wic_factory));
    if (FAILED(hr)) {
        _com_error err(hr);
        std::cerr << err.ErrorMessage() << '\n';
    }

    ComPtr<IDWriteFontCollection> font_collection;
    pimpl->dwrite_factory->GetSystemFontCollection(&font_collection);

    // https://stackoverflow.com/q/40365439/14698275
    UINT32 index;
    BOOL exists;
    font_collection->FindFamilyName(pimpl->font_name_utf16.data(), &index, &exists);

    ComPtr<IDWriteFontFamily> font_family;
    font_collection->GetFontFamily(index, &font_family);

    ComPtr<IDWriteFont> font;
    font_family->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STRETCH_NORMAL,
                                      DWRITE_FONT_STYLE_NORMAL, &font);

    font->CreateFontFace(&pimpl->font_face);

    // TODO: Verify that this is correct.
    pimpl->em_size = static_cast<FLOAT>(font_size) * 96 / 72;

    DWRITE_FONT_METRICS metrics;
    pimpl->font_face->GetMetrics(&metrics);

    FLOAT scale = pimpl->em_size / metrics.designUnitsPerEm;

    int ascent = std::ceil(metrics.ascent * scale);
    int descent = std::ceil(-metrics.descent * scale);
    int line_gap = std::ceil(metrics.lineGap * scale);
    int line_height = ascent - descent + line_gap;

    // TODO: Remove magic numbers that emulate Sublime Text.
    line_height += 1;

    this->line_height = line_height;
    this->descent = descent;
}

FontRasterizer::~FontRasterizer() {}

FontRasterizer::RasterizedGlyph FontRasterizer::rasterizeUTF8(size_t font_id,
                                                              uint32_t glyph_id) const {
    // TODO: Implement font_id lookup.
    ComPtr<IDWriteFontFace> font_face = pimpl->font_face;
    UINT16 glyph_index = glyph_id;

    FLOAT glyph_advances = 0;
    DWRITE_GLYPH_OFFSET offset{};
    DWRITE_GLYPH_RUN glyph_run{
        .fontFace = font_face.Get(),
        .fontEmSize = pimpl->em_size,
        .glyphCount = 1,
        .glyphIndices = &glyph_index,
        .glyphAdvances = &glyph_advances,
        .glyphOffsets = &offset,
        .isSideways = 0,
        .bidiLevel = 0,
    };

    IDWriteRenderingParams* rendering_params;
    pimpl->dwrite_factory->CreateRenderingParams(&rendering_params);

    DWRITE_RENDERING_MODE rendering_mode;
    font_face->GetRecommendedRenderingMode(pimpl->em_size, 1.0, DWRITE_MEASURING_MODE_NATURAL,
                                           rendering_params, &rendering_mode);

    IDWriteGlyphRunAnalysis* glyph_run_analysis;
    pimpl->dwrite_factory->CreateGlyphRunAnalysis(&glyph_run, 1.0, nullptr, rendering_mode,
                                                  DWRITE_MEASURING_MODE_NATURAL, 0.0, 0.0,
                                                  &glyph_run_analysis);

    RECT texture_bounds;
    glyph_run_analysis->GetAlphaTextureBounds(DWRITE_TEXTURE_CLEARTYPE_3x1, &texture_bounds);

    LONG pixel_width = texture_bounds.right - texture_bounds.left;
    LONG pixel_height = texture_bounds.bottom - texture_bounds.top;
    UINT32 size = pixel_width * pixel_height * 3;

    // std::cerr << std::format("pixel_width = {}, pixel_height = {}\n", pixel_width,
    // pixel_height);

    DWRITE_GLYPH_METRICS metrics;
    font_face->GetDesignGlyphMetrics(&glyph_index, 1, &metrics, false);

    DWRITE_FONT_METRICS font_metrics;
    font_face->GetMetrics(&font_metrics);

    FLOAT scale = pimpl->em_size / font_metrics.designUnitsPerEm;
    FLOAT advance = metrics.advanceWidth * scale;

    int32_t top = -texture_bounds.top;
    top -= descent;

    HRESULT hr = DWRITE_E_NOCOLOR;
    ComPtr<IDWriteColorGlyphRunEnumerator1> color_run_enumerator;

    ComPtr<IDWriteFontFace2> font_face_2;
    font_face->QueryInterface(reinterpret_cast<IDWriteFontFace2**>(font_face_2.GetAddressOf()));
    if (font_face_2->IsColorFont()) {
        DWRITE_GLYPH_IMAGE_FORMATS image_formats = DWRITE_GLYPH_IMAGE_FORMATS_COLR;
        hr = pimpl->dwrite_factory->TranslateColorGlyphRun({}, &glyph_run, nullptr, image_formats,
                                                           DWRITE_MEASURING_MODE_NATURAL, nullptr,
                                                           0, &color_run_enumerator);
    }

    // Non-colored glyph run.
    if (hr == DWRITE_E_NOCOLOR) {
        std::vector<uint8_t> buffer(size);
        glyph_run_analysis->CreateAlphaTexture(DWRITE_TEXTURE_CLEARTYPE_3x1, &texture_bounds,
                                               &buffer[0], size);

        return {
            .colored = false,
            .left = texture_bounds.left,
            .top = top,
            .width = static_cast<int32_t>(pixel_width),
            .height = static_cast<int32_t>(pixel_height),
            .advance = static_cast<int32_t>(std::ceil(advance)),
            .buffer = std::move(buffer),
        };
    }
    // Colored glyph run.
    else {
        ComPtr<IWICBitmap> wic_bitmap;
        // TODO: Implement without magic numbers. Properly find the right width/height.
        UINT bitmap_width = pixel_width + 10;
        UINT bitmap_height = pixel_height + 10;
        pimpl->wic_factory->CreateBitmap(bitmap_width, bitmap_height,
                                         GUID_WICPixelFormat32bppPRGBA, WICBitmapCacheOnDemand,
                                         wic_bitmap.GetAddressOf());

        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();

        // TODO: Find a way to reuse render target and brushes.
        ComPtr<ID2D1RenderTarget> target;
        pimpl->d2d_factory->CreateWicBitmapRenderTarget(wic_bitmap.Get(), props,
                                                        target.GetAddressOf());

        pimpl->drawColorRun(target.Get(), std::move(color_run_enumerator), -texture_bounds.top);

        IWICBitmapLock* bitmap_lock;
        wic_bitmap.Get()->Lock(nullptr, WICBitmapLockRead, &bitmap_lock);

        UINT buffer_size = 0;
        BYTE* pv = NULL;
        bitmap_lock->GetDataPointer(&buffer_size, &pv);

        UINT bw = 0, bh = 0;
        bitmap_lock->GetSize(&bw, &bh);
        size_t pixels = bw * bh;

        std::vector<uint8_t> temp_buffer;
        temp_buffer.reserve(pixels * 4);
        for (size_t i = 0; i < pixels; ++i) {
            size_t offset = i * 4;
            temp_buffer.emplace_back(pv[offset]);
            temp_buffer.emplace_back(pv[offset + 1]);
            temp_buffer.emplace_back(pv[offset + 2]);
            temp_buffer.emplace_back(pv[offset + 3]);
        }

        bitmap_lock->Release();

        return {
            .colored = true,
            .left = 0,
            .top = top,
            .width = static_cast<int32_t>(bw),
            .height = static_cast<int32_t>(bh),
            .advance = static_cast<int32_t>(std::ceil(advance)),
            .buffer = std::move(temp_buffer),
        };
    }
}

FontRasterizer::LineLayout FontRasterizer::layoutLine(std::string_view str8) const {
    std::wstring str16 = base::windows::ConvertToUTF16(str8);

    IDWriteFontCollection* font_collection;
    pimpl->dwrite_factory->GetSystemFontCollection(&font_collection);

    ComPtr<IDWriteTextFormat> text_format;
    pimpl->dwrite_factory->CreateTextFormat(pimpl->font_name_utf16.data(), font_collection,
                                            DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL,
                                            DWRITE_FONT_STRETCH_NORMAL, pimpl->em_size, L"en-us",
                                            &text_format);

    UINT32 len = str16.length();
    ComPtr<IDWriteTextLayout> text_layout;
    pimpl->dwrite_factory->CreateTextLayout(str16.data(), len, text_format.Get(), 200.0f, 200.0f,
                                            &text_layout);

    ComPtr<FontFallbackRenderer> font_fallback_renderer = new FontFallbackRenderer{};

    text_layout->SetFontCollection(font_collection, {0, len});
    text_layout->Draw(nullptr, font_fallback_renderer.Get(), 50.0f, 50.0f);

    return {
        .width = font_fallback_renderer->total_advance,
        .length = str8.length(),
        .runs = font_fallback_renderer->runs,
    };
}

void FontRasterizer::impl::drawColorRun(
    ID2D1RenderTarget* target,
    ComPtr<IDWriteColorGlyphRunEnumerator1> color_run_enumerator,
    UINT origin_y) {
    // TODO: Find a way to reuse render target and brushes.
    ComPtr<ID2D1SolidColorBrush> blue_brush = nullptr;
    target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Blue, 1.0f), &blue_brush);

    D2D1_POINT_2F baseline_origin{
        .x = 0,
        .y = static_cast<FLOAT>(origin_y),
    };

    target->BeginDraw();
    BOOL has_run;
    const DWRITE_COLOR_GLYPH_RUN1* color_run;

    while (true) {
        if (FAILED(color_run_enumerator->MoveNext(&has_run)) || !has_run) {
            break;
        }
        if (FAILED(color_run_enumerator->GetCurrentRun(&color_run))) {
            break;
        }

        switch (color_run->glyphImageFormat) {
        case DWRITE_GLYPH_IMAGE_FORMATS_PNG:
        case DWRITE_GLYPH_IMAGE_FORMATS_JPEG:
        case DWRITE_GLYPH_IMAGE_FORMATS_TIFF:
        case DWRITE_GLYPH_IMAGE_FORMATS_PREMULTIPLIED_B8G8R8A8:
            // std::cerr << "DrawColorBitmapGlyphRun()\n";
            break;

        case DWRITE_GLYPH_IMAGE_FORMATS_SVG:
            // std::cerr << "DrawSvgGlyphRun()\n";
            break;

        case DWRITE_GLYPH_IMAGE_FORMATS_TRUETYPE:
        case DWRITE_GLYPH_IMAGE_FORMATS_CFF:
        case DWRITE_GLYPH_IMAGE_FORMATS_COLR:
        default: {
            // std::cerr << "DrawGlyphRun()\n";

            ComPtr<ID2D1SolidColorBrush> layer_brush;
            if (color_run->paletteIndex == 0xFFFF) {
                layer_brush = blue_brush;
            } else {
                target->CreateSolidColorBrush(color_run->runColor, &layer_brush);
            }

            target->DrawGlyphRun(baseline_origin, &color_run->glyphRun, layer_brush.Get());
            break;
        }
        }
    }
    target->EndDraw();
}

}
