#include "base/windows/unicode.h"
#include "font/directwrite/directwrite_helper.h"
#include "font/directwrite/font_fallback_renderer.h"
#include "font/directwrite/impl_directwrite.h"
#include "font/font_rasterizer.h"
#include <algorithm>
#include <combaseapi.h>
#include <comdef.h>
#include <cwchar>
#include <d2d1.h>
#include <unknwnbase.h>
#include <vector>
#include <wincodec.h>
#include <winerror.h>
#include <wingdi.h>
#include <winnt.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

// TODO: Debug use; remove this.
#include <fmt/base.h>
#include <cassert>

namespace font {

namespace {
inline std::string PostScriptName(ComPtr<IDWriteFont> font);
inline void DrawColorRun(
    ID2D1RenderTarget* target,
    Microsoft::WRL::ComPtr<IDWriteColorGlyphRunEnumerator1> color_run_enumerator,
    UINT origin_y);
}  // namespace

FontRasterizer::FontRasterizer() : pimpl{new impl{}} {
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory4),
                        reinterpret_cast<IUnknown**>(pimpl->dwrite_factory.GetAddressOf()));
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, pimpl->d2d_factory.GetAddressOf());

    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pimpl->wic_factory));
    if (FAILED(hr)) {
        // TODO: Make this work with `UNICODE`/`_UNICODE`.
        // _com_error err(hr);
        // fmt::println(err.ErrorMessage());
        std::abort();
    }
}

FontRasterizer::~FontRasterizer() {}

size_t FontRasterizer::addFont(std::string_view font_name8, int font_size, FontStyle style) {
    std::wstring font_name16 = base::windows::ConvertToUTF16(font_name8);

    ComPtr<IDWriteFontCollection> font_collection;
    pimpl->dwrite_factory->GetSystemFontCollection(&font_collection);

    // https://stackoverflow.com/q/40365439/14698275
    UINT32 index;
    BOOL exists;
    HRESULT hr;
    hr = font_collection->FindFamilyName(font_name16.data(), &index, &exists);
    if (FAILED(hr)) {
        fmt::println("Could not create font family with name {} and size {}.", font_name8,
                     font_size);
    }

    ComPtr<IDWriteFontFamily> font_family;
    hr = font_collection->GetFontFamily(index, &font_family);
    if (FAILED(hr)) {
        fmt::println("Could not create font family with name {} and size {}.", font_name8,
                     font_size);
    }

    ComPtr<IDWriteFont> font;
    hr = font_family->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STRETCH_NORMAL,
                                           DWRITE_FONT_STYLE_NORMAL, &font);
    if (FAILED(hr)) {
        fmt::println("Could not create font with name {} and size {}.", font_name8, font_size);
    }

    return cacheFont({font}, font_size);
}

size_t FontRasterizer::addSystemFont(int font_size, FontStyle style) {
    NONCLIENTMETRICS ncm = {
        .cbSize = sizeof(ncm),
    };
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);

    IDWriteGdiInterop* gdi_interop = NULL;
    pimpl->dwrite_factory->GetGdiInterop(&gdi_interop);

    ComPtr<IDWriteFont> sys_font;
    gdi_interop->CreateFontFromLOGFONT(&ncm.lfMessageFont, &sys_font);
    return cacheFont({sys_font}, font_size);
}

RasterizedGlyph FontRasterizer::rasterize(size_t font_id, uint32_t glyph_id) const {
    ComPtr<IDWriteFont> font = font_id_to_native[font_id].font;
    const auto& dwrite_info = pimpl->font_id_to_dwrite_info[font_id];

    ComPtr<IDWriteFontFace> font_face;
    font->CreateFontFace(&font_face);
    UINT16 glyph_index = glyph_id;

    FLOAT glyph_advances = 0;
    DWRITE_GLYPH_OFFSET offset{};
    DWRITE_GLYPH_RUN glyph_run{
        .fontFace = font_face.Get(),
        .fontEmSize = dwrite_info.em_size,
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
    font_face->GetRecommendedRenderingMode(dwrite_info.em_size, 1.0, DWRITE_MEASURING_MODE_NATURAL,
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

    DWRITE_GLYPH_METRICS metrics;
    font_face->GetDesignGlyphMetrics(&glyph_index, 1, &metrics, false);

    DWRITE_FONT_METRICS font_metrics;
    font_face->GetMetrics(&font_metrics);

    int32_t top = -texture_bounds.top;

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
                                               buffer.data(), size);

        return {
            .colored = false,
            .left = texture_bounds.left,
            .top = top,
            .width = static_cast<int32_t>(pixel_width),
            .height = static_cast<int32_t>(pixel_height),
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
        DrawColorRun(target.Get(), std::move(color_run_enumerator), -texture_bounds.top);

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
            .buffer = std::move(temp_buffer),
        };
    }
}

LineLayout FontRasterizer::layoutLine(size_t font_id, std::string_view str8) {
    assert(std::ranges::count(str8, '\n') == 0);

    std::wstring str16 = base::windows::ConvertToUTF16(str8);

    const auto& dwrite_info = pimpl->font_id_to_dwrite_info[font_id];

    IDWriteFontCollection* font_collection;
    pimpl->dwrite_factory->GetSystemFontCollection(&font_collection);

    ComPtr<IDWriteTextFormat> text_format;
    pimpl->dwrite_factory->CreateTextFormat(dwrite_info.font_name16.data(), font_collection,
                                            DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL,
                                            DWRITE_FONT_STRETCH_NORMAL, dwrite_info.em_size,
                                            L"en-us", &text_format);

    // TODO: The `maxWidth`/`maxHeight` arguments *do* prevent ligature formation if the values are
    // too low. Find out what the appropriate values are.
    // You can reproduce this by creating a long string of `=` in Cascadia/Fira Code. Eventually,
    // the ligatures break when they go past the max width.
    UINT32 len = str16.length();
    ComPtr<IDWriteTextLayout> text_layout;
    pimpl->dwrite_factory->CreateTextLayout(str16.data(), len, text_format.Get(), 2000.0f, 2000.0f,
                                            &text_layout);

    // OpenType features.
    // TODO: Consider using the lower-level IDWriteTextAnalyzer, which IDWriteTextLayout uses under
    // the hood. IDWriteTextLayout::CreateTypography() removes the default per-script OpenType
    // options, which is not ideal.
    // https://stackoverflow.com/questions/32545675/what-are-the-default-typography-settings-used-by-idwritetextlayout#48800921
    // https://stackoverflow.com/questions/44611592/how-do-i-balance-script-oriented-opentype-features-with-other-opentype-features

    IDWriteTypography* typography;
    pimpl->dwrite_factory->CreateTypography(&typography);
    // Since we have to provide our own defaults, here are some sane ones from Harfbuzz:
    // https://github.com/harfbuzz/harfbuzz/blob/f35b0a63b1c30923e91b612399c4387e64432b91/src/hb-ot-shape.cc#L285-L308
    typography->AddFontFeature({DWRITE_FONT_FEATURE_TAG_GLYPH_COMPOSITION_DECOMPOSITION, 1});
    typography->AddFontFeature({DWRITE_FONT_FEATURE_TAG_LOCALIZED_FORMS, 1});
    typography->AddFontFeature({DWRITE_FONT_FEATURE_TAG_MARK_POSITIONING, 1});
    typography->AddFontFeature({DWRITE_FONT_FEATURE_TAG_MARK_TO_MARK_POSITIONING, 1});
    typography->AddFontFeature({DWRITE_FONT_FEATURE_TAG_REQUIRED_LIGATURES, 1});
    typography->AddFontFeature({DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_ALTERNATES, 1});
    typography->AddFontFeature({DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_LIGATURES, 1});
    typography->AddFontFeature({DWRITE_FONT_FEATURE_TAG_CURSIVE_POSITIONING, 1});
    typography->AddFontFeature({DWRITE_FONT_FEATURE_TAG_KERNING, 1});
    typography->AddFontFeature({DWRITE_FONT_FEATURE_TAG_STANDARD_LIGATURES, 1});
    // Additional tags.
    typography->AddFontFeature({DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_19, 1});
    text_layout->SetTypography(typography, {0, len});

    text_layout->SetFontCollection(font_collection, {0, len});

    ComPtr<FontFallbackRenderer> font_fallback_renderer =
        new FontFallbackRenderer{font_collection, str8};
    text_layout->Draw(this, font_fallback_renderer.Get(), 0.0f, 0.0f);

    return {
        .layout_font_id = font_id,
        .width = font_fallback_renderer->total_advance,
        .length = str8.length(),
        .runs = font_fallback_renderer->runs,
    };
}

size_t FontRasterizer::cacheFont(NativeFontType font, int font_size) {
    ComPtr<IDWriteFont> dwrite_font = font.font;
    std::string postscript_name = PostScriptName(dwrite_font);

    if (!font_postscript_name_to_id.contains(postscript_name)) {
        ComPtr<IDWriteFontFace> font_face;
        dwrite_font->CreateFontFace(&font_face);

        DWRITE_FONT_METRICS dwrite_metrics;
        font_face->GetMetrics(&dwrite_metrics);

        float em_size = font_size * 96.f / 72;
        float scale = em_size / dwrite_metrics.designUnitsPerEm;

        int ascent = std::ceil(dwrite_metrics.ascent * scale);
        int descent = std::ceil(-dwrite_metrics.descent * scale);

        // Round up to the next even number if odd.
        if (ascent % 2 == 1) ++ascent;
        if (descent % 2 == 1) ++descent;

        int line_gap = std::ceil(dwrite_metrics.lineGap * scale);
        int line_height = ascent - descent + line_gap;

        Metrics metrics{
            .line_height = line_height,
            .ascent = ascent,
            .descent = descent,
            .font_size = font_size,
        };
        impl::DWriteInfo dwrite_info{
            .font_name16 = GetFontFamilyName(dwrite_font.Get()),
            .em_size = em_size,
        };

        size_t font_id = font_id_to_native.size();
        font_postscript_name_to_id.emplace(postscript_name, font_id);
        font_id_to_native.emplace_back(font);
        font_id_to_metrics.emplace_back(std::move(metrics));
        pimpl->font_id_to_dwrite_info.emplace_back(std::move(dwrite_info));
    }
    return font_postscript_name_to_id.at(postscript_name);
}

namespace {
std::string PostScriptName(ComPtr<IDWriteFont> font) {
    ComPtr<IDWriteLocalizedStrings> font_id_keyed_names;
    BOOL has_id_keyed_names;
    font->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_NAME,
                                  &font_id_keyed_names, &has_id_keyed_names);

    wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
    GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH);

    UINT32 index = 0;
    BOOL exists = false;
    font_id_keyed_names->FindLocaleName(localeName, &index, &exists);

    UINT32 length = 0;
    font_id_keyed_names->GetStringLength(index, &length);

    std::wstring localized_name;
    localized_name.resize(length + 1);
    font_id_keyed_names->GetString(index, localized_name.data(), length + 1);
    return base::windows::ConvertToUTF8(localized_name);
}

void DrawColorRun(ID2D1RenderTarget* target,
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
            fmt::println("Error: DrawColorBitmapGlyphRun() is unimplemented!");
            std::abort();
            break;

        case DWRITE_GLYPH_IMAGE_FORMATS_SVG:
            fmt::println("Error: DrawSvgGlyphRun() is unimplemented!");
            std::abort();
            break;

        case DWRITE_GLYPH_IMAGE_FORMATS_TRUETYPE:
        case DWRITE_GLYPH_IMAGE_FORMATS_CFF:
        case DWRITE_GLYPH_IMAGE_FORMATS_COLR:
        default: {
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
}  // namespace

}  // namespace font
