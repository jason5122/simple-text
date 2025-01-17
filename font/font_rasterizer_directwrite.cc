#include "font/font_rasterizer.h"

#include <cwchar>
#include <limits>
#include <vector>

#include <combaseapi.h>
#include <comdef.h>
#include <d2d1_3.h>
#include <dwrite_3.h>
#include <unknwnbase.h>
#include <wincodec.h>
#include <winerror.h>
#include <wingdi.h>
#include <winnt.h>
#include <wrl/client.h>

#include "base/windows/unicode.h"
#include "font/font_fallback_renderer.h"
#include "font/impl_directwrite.h"

using Microsoft::WRL::ComPtr;

// TODO: Debug use; remove this.
#include <cassert>
#include <fmt/base.h>
#include <fmt/xchar.h>

namespace font {

namespace {

inline void DrawColorRun(ID2D1RenderTarget* target,
                         IDWriteColorGlyphRunEnumerator1* color_run_enumerator,
                         const D2D1_POINT_2F& baseline_origin);
inline std::wstring GetPostscriptName(IDWriteFont* font, std::wstring_view locale);
inline std::wstring GetFontFamilyName(IDWriteFont* font, std::wstring_view locale);
inline std::wstring GetLocaleName(IDWriteLocalizedStrings* strings, std::wstring_view locale);

}  // namespace

FontRasterizer::FontRasterizer() : pimpl(new impl()) {
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory4),
                        reinterpret_cast<IUnknown**>(pimpl->dwrite_factory.GetAddressOf()));
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, pimpl->d2d1_factory.GetAddressOf());

    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pimpl->wic_factory));
    if (FAILED(hr)) {
        // TODO: Make this work with `UNICODE`/`_UNICODE`.
        // _com_error err(hr);
        // fmt::println(err.ErrorMessage());
        std::abort();
    }

    WCHAR locale_storage[LOCALE_NAME_MAX_LENGTH];
    GetUserDefaultLocaleName(locale_storage, LOCALE_NAME_MAX_LENGTH);
    pimpl->locale = std::wstring(locale_storage);

    ComPtr<IDWriteRenderingParams> default_params;
    pimpl->dwrite_factory->CreateRenderingParams(default_params.GetAddressOf());
    FLOAT gamma = default_params->GetGamma();
    FLOAT enhanced_contrast = default_params->GetEnhancedContrast();
    FLOAT cleartype_level = default_params->GetClearTypeLevel();
    // TODO: See if we should hard code pixel geometry and rendering mode.
    ComPtr<IDWriteRenderingParams> params;
    pimpl->dwrite_factory->CreateCustomRenderingParams(
        gamma, enhanced_contrast, cleartype_level, DWRITE_PIXEL_GEOMETRY_RGB,
        DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC, params.GetAddressOf());

    D2D1_RENDER_TARGET_PROPERTIES render_target_properties = {
        .type = D2D1_RENDER_TARGET_TYPE_DEFAULT,
        .pixelFormat =
            {
                .format = DXGI_FORMAT_B8G8R8A8_UNORM,
                .alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED,
            },
        .dpiX = 96.0,
        .dpiY = 96.0,
        .usage = D2D1_RENDER_TARGET_USAGE_NONE,
        .minLevel = D2D1_FEATURE_LEVEL_DEFAULT,
    };
    pimpl->d2d1_factory->CreateDCRenderTarget(&render_target_properties,
                                              pimpl->dc_target.GetAddressOf());
    pimpl->dc_target->SetTextRenderingParams(params.Get());
}

FontRasterizer::~FontRasterizer() {}

FontId FontRasterizer::add_font(std::string_view font_name8, int font_size, FontStyle style) {
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

    return cache_font({font}, font_size);
}

FontId FontRasterizer::add_system_font(int font_size, FontStyle style) {
    NONCLIENTMETRICS metrics = {};
    metrics.cbSize = sizeof(metrics);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, metrics.cbSize, &metrics, 0);

    IDWriteGdiInterop* gdi_interop = nullptr;
    pimpl->dwrite_factory->GetGdiInterop(&gdi_interop);

    ComPtr<IDWriteFont> sys_font;
    gdi_interop->CreateFontFromLOGFONT(&metrics.lfMessageFont, &sys_font);
    return cache_font({sys_font}, font_size);
}

// TODO: Implement this.
FontId FontRasterizer::resize_font(FontId font_id, int font_size) {
    fmt::println("Warning: Implement FontRasterizer::resize_font()");
    return font_id;
}

RasterizedGlyph FontRasterizer::rasterize(FontId font_id, uint32_t glyph_id) const {
    ComPtr<IDWriteFont> font = font_id_to_native[font_id].font;
    const auto& dwrite_info = pimpl->font_id_to_dwrite_info[font_id];

    ComPtr<IDWriteFontFace> font_face;
    font->CreateFontFace(&font_face);

    UINT16 glyph_index = glyph_id;
    FLOAT advance = 0.0;
    DWRITE_GLYPH_OFFSET offset{};
    DWRITE_GLYPH_RUN glyph_run = {
        .fontFace = font_face.Get(),
        .fontEmSize = dwrite_info.em_size,
        .glyphCount = 1,
        .glyphIndices = &glyph_index,
        .glyphAdvances = &advance,
        .glyphOffsets = &offset,
        .isSideways = 0,
        .bidiLevel = 0,
    };

    // TODO: Debug this.
    ComPtr<ID2D1DeviceContext4> dc_target4;
    pimpl->dc_target->QueryInterface(
        reinterpret_cast<ID2D1DeviceContext4**>(dc_target4.GetAddressOf()));
    dc_target4->SetUnitMode(D2D1_UNIT_MODE_DIPS);
    dc_target4->SetDpi(96.0, 96.0);
    D2D1_RECT_F bounds;
    dc_target4->GetGlyphRunWorldBounds({}, &glyph_run, DWRITE_MEASURING_MODE_NATURAL, &bounds);

    ComPtr<IDWriteRenderingParams> rendering_params;
    pimpl->dwrite_factory->CreateRenderingParams(rendering_params.GetAddressOf());

    DWRITE_RENDERING_MODE rendering_mode;
    font_face->GetRecommendedRenderingMode(dwrite_info.em_size, 1.0, DWRITE_MEASURING_MODE_NATURAL,
                                           rendering_params.Get(), &rendering_mode);

    ComPtr<IDWriteGlyphRunAnalysis> glyph_run_analysis;
    pimpl->dwrite_factory->CreateGlyphRunAnalysis(&glyph_run, 1.0, nullptr, rendering_mode,
                                                  DWRITE_MEASURING_MODE_NATURAL, 0.0, 0.0,
                                                  glyph_run_analysis.GetAddressOf());

    RECT texture_bounds;
    glyph_run_analysis->GetAlphaTextureBounds(DWRITE_TEXTURE_CLEARTYPE_3x1, &texture_bounds);

    // TODO: Debug use; remove this.
    fmt::println("font = {}, glyph = {}, left = {} vs {}, top = {} vs {}, right = {} vs {}, "
                 "bottom = {} vs {}",
                 font_id, glyph_id, bounds.left, texture_bounds.left, bounds.top,
                 texture_bounds.top, bounds.right, texture_bounds.right, bounds.bottom,
                 texture_bounds.bottom);
    texture_bounds.left = std::ceil(bounds.left);
    texture_bounds.top = std::ceil(bounds.top);
    texture_bounds.right = std::ceil(bounds.right);
    texture_bounds.bottom = std::ceil(bounds.bottom);

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

        // TODO: Clean this up.
        size_t pixels = pixel_width * pixel_height;
        std::vector<uint8_t> bitmap_data;
        bitmap_data.reserve(pixels * 4);
        for (size_t i = 0; i < pixels; ++i) {
            size_t offset = i * 3;
            bitmap_data.emplace_back(buffer[offset]);
            bitmap_data.emplace_back(buffer[offset]);
            bitmap_data.emplace_back(buffer[offset]);
            bitmap_data.emplace_back(buffer[offset]);
        }

        return {
            .left = texture_bounds.left,
            .top = top,
            .width = static_cast<int32_t>(pixel_width),
            .height = static_cast<int32_t>(pixel_height),
            // .buffer = std::move(buffer),
            .buffer = std::move(bitmap_data),
            .colored = false,
        };
    }
    // Colored glyph run.
    else {
        ComPtr<IWICBitmap> wic_bitmap;
        // TODO: Implement without magic numbers. Properly find the right width/height.
        UINT width = pixel_width + 10;
        UINT height = pixel_height + 10;
        pimpl->wic_factory->CreateBitmap(width, height, GUID_WICPixelFormat32bppPBGRA,
                                         WICBitmapCacheOnDemand, wic_bitmap.GetAddressOf());

        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();

        // TODO: Find a way to reuse render target and brushes.
        ComPtr<ID2D1RenderTarget> target;
        pimpl->d2d1_factory->CreateWicBitmapRenderTarget(wic_bitmap.Get(), props,
                                                         target.GetAddressOf());
        D2D1_POINT_2F baseline_origin = {
            .x = 0,
            .y = static_cast<FLOAT>(-texture_bounds.top),
        };
        DrawColorRun(target.Get(), color_run_enumerator.Get(), baseline_origin);

        // TODO: Try to use the bitmap data without copying/manipulating the pixels.
        // TODO: Change stride based on plain/colored text.
        UINT stride = width * 4;
        std::vector<uint8_t> bitmap_data(stride * height);
        wic_bitmap->CopyPixels(nullptr, stride, bitmap_data.size(), bitmap_data.data());

        return {
            .left = 0,
            .top = top,
            .width = static_cast<int32_t>(width),
            .height = static_cast<int32_t>(height),
            .buffer = std::move(bitmap_data),
            .colored = true,
        };
    }
}

LineLayout FontRasterizer::layout_line(FontId font_id, std::string_view str8) {
    assert(str8.find('\n') == std::string_view::npos);

    std::wstring str16 = base::windows::ConvertToUTF16(str8);

    const auto& dwrite_info = pimpl->font_id_to_dwrite_info[font_id];

    IDWriteFontCollection* font_collection;
    pimpl->dwrite_factory->GetSystemFontCollection(&font_collection);

    ComPtr<IDWriteTextFormat> text_format;
    pimpl->dwrite_factory->CreateTextFormat(dwrite_info.font_name16.data(), font_collection,
                                            DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL,
                                            DWRITE_FONT_STRETCH_NORMAL, dwrite_info.em_size,
                                            pimpl->locale.data(), &text_format);

    UINT32 len = str16.length();
    ComPtr<IDWriteTextLayout> text_layout;
    pimpl->dwrite_factory->CreateTextLayout(str16.data(), len, text_format.Get(),
                                            std::numeric_limits<float>::infinity(),
                                            std::numeric_limits<float>::infinity(), &text_layout);

    // OpenType features.
    // TODO: Consider using the lower-level IDWriteTextAnalyzer, which IDWriteTextLayout uses under
    // the hood. IDWriteTextLayout::CreateTypography() removes the default per-script OpenType
    // options, which is not ideal.
    // https://stackoverflow.com/questions/32545675/what-are-the-default-typography-settings-used-by-idwritetextlayout#48800921
    // https://stackoverflow.com/questions/44611592/how-do-i-balance-script-oriented-opentype-features-with-other-opentype-features

    ComPtr<IDWriteTypography> typography;
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
    text_layout->SetTypography(typography.Get(), {0, len});

    text_layout->SetFontCollection(font_collection, {0, len});

    ComPtr<FontFallbackRenderer> font_fallback_renderer =
        new FontFallbackRenderer(font_collection, str8);
    text_layout->Draw(this, font_fallback_renderer.Get(), 0.0f, 0.0f);

    return {
        .layout_font_id = font_id,
        .width = font_fallback_renderer->total_advance,
        .length = str8.length(),
        .glyphs = font_fallback_renderer->glyphs,
    };
}

FontId FontRasterizer::cache_font(NativeFontType native_font, int font_size) {
    ComPtr<IDWriteFont> dwrite_font = native_font.font;
    std::wstring font_name = GetPostscriptName(dwrite_font.Get(), pimpl->locale);

    // If the font is already present, return its ID.
    size_t hash = hash_font(font_name, font_size);
    if (auto it = font_hash_to_id.find(hash); it != font_hash_to_id.end()) {
        return it->second;
    }

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

    Metrics metrics = {
        .line_height = line_height,
        .ascent = ascent,
        .descent = descent,
        .font_size = font_size,
    };
    impl::DWriteInfo dwrite_info = {
        .font_name16 = GetFontFamilyName(dwrite_font.Get(), pimpl->locale),
        .em_size = em_size,
    };

    FontId font_id = font_id_to_native.size();
    font_hash_to_id.emplace(hash, font_id);
    font_id_to_native.emplace_back(std::move(native_font));
    font_id_to_metrics.emplace_back(std::move(metrics));
    // TODO: See if we can prevent this conversion.
    std::string font_name8 = base::windows::ConvertToUTF8(font_name);
    font_id_to_postscript_name.emplace_back(std::move(font_name8));
    pimpl->font_id_to_dwrite_info.emplace_back(std::move(dwrite_info));
    return font_id;
}

namespace {

void DrawColorRun(ID2D1RenderTarget* target,
                  IDWriteColorGlyphRunEnumerator1* color_run_enumerator,
                  const D2D1_POINT_2F& baseline_origin) {
    ComPtr<ID2D1SolidColorBrush> brush;
    target->CreateSolidColorBrush({1.0f, 1.0f, 1.0f, 1.0f}, &brush);

    target->BeginDraw();
    while (true) {
        BOOL has_run;
        const DWRITE_COLOR_GLYPH_RUN1* color_run;
        if (FAILED(color_run_enumerator->MoveNext(&has_run)) || !has_run) {
            break;
        }
        if (FAILED(color_run_enumerator->GetCurrentRun(&color_run))) {
            break;
        }

        switch (color_run->glyphImageFormat) {
        case DWRITE_GLYPH_IMAGE_FORMATS_COLR:
            brush->SetColor(color_run->runColor);
            target->DrawGlyphRun(baseline_origin, &color_run->glyphRun, brush.Get(),
                                 color_run->measuringMode);
            break;
        default:
            fmt::println("Error: DirectWrite glyph image format unimplemented");
            std::abort();
        }
    }
    target->EndDraw();
}

std::wstring GetPostscriptName(IDWriteFont* font, std::wstring_view locale) {
    ComPtr<IDWriteLocalizedStrings> font_id_keyed_names;
    BOOL has_id_keyed_names;
    font->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_NAME,
                                  &font_id_keyed_names, &has_id_keyed_names);
    return GetLocaleName(font_id_keyed_names.Get(), locale);
}

std::wstring GetFontFamilyName(IDWriteFont* font, std::wstring_view locale) {
    ComPtr<IDWriteFontFamily> font_family;
    font->GetFontFamily(&font_family);

    ComPtr<IDWriteLocalizedStrings> family_names;
    font_family->GetFamilyNames(&family_names);
    return GetLocaleName(family_names.Get(), locale);
}

std::wstring GetLocaleName(IDWriteLocalizedStrings* strings, std::wstring_view locale) {
    // Follow recommended strategy for getting locale name.
    // https://learn.microsoft.com/en-us/windows/win32/api/dwrite/nn-dwrite-idwritelocalizedstrings
    UINT32 index = 0;
    BOOL exists = false;
    strings->FindLocaleName(locale.data(), &index, &exists);
    // If the above find did not find a match, retry with US English.
    if (!exists) {
        strings->FindLocaleName(L"en-US", &index, &exists);
    }
    // If the specified locale doesn't exist, select the first on the list.
    if (!exists) {
        index = 0;
    }

    UINT32 length = 0;
    strings->GetStringLength(index, &length);

    std::wstring name;
    name.resize(length + 1);
    strings->GetString(index, name.data(), length + 1);
    return name;
}

}  // namespace

}  // namespace font
