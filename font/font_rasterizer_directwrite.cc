#include "base/check.h"
#include "base/windows/unicode.h"
#include "font/font_rasterizer.h"
#include "unicode/utf16_to_utf8_indices_map.h"
#include <combaseapi.h>
#include <comdef.h>
#include <cwchar>
#include <d2d1_3.h>
#include <dwrite_3.h>
#include <limits>
#include <spdlog/spdlog.h>
#include <unknwnbase.h>
#include <vector>
#include <wincodec.h>
#include <winerror.h>
#include <wingdi.h>
#include <winnt.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace font {

namespace {

inline std::wstring GetPostscriptName(IDWriteFont* font, std::wstring_view locale);
inline std::wstring GetFontFamilyName(IDWriteFont* font, std::wstring_view locale);
inline std::wstring GetLocaleName(IDWriteLocalizedStrings* strings, std::wstring_view locale);

}  // namespace

struct FontRasterizer::NativeFontType {
    ComPtr<IDWriteFont> font;
};

class FontRasterizer::Impl {
public:
    ComPtr<IDWriteFactory4> dwrite_factory;
    ComPtr<ID2D1Factory> d2d1_factory;
    ComPtr<IWICImagingFactory2> wic_factory;
    ComPtr<ID2D1DCRenderTarget> dc_target;
    ComPtr<IDWriteRenderingParams> text_rendering_params;

    struct DWriteInfo {
        std::wstring font_name16;
        float em_size;
    };
    std::vector<DWriteInfo> font_id_to_dwrite_info;
    std::wstring locale;
};

FontRasterizer::FontRasterizer() : pimpl(new Impl()) {
    // TODO: Check return values here and handle errors.

    // Create DirectWrite factory.
    ComPtr<IUnknown> factory_unknown;
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory4), &factory_unknown);
    factory_unknown.CopyTo(&pimpl->dwrite_factory);
    // Create D2D1 factory.
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory),
                      &pimpl->d2d1_factory);
    // Create WIC bitmap factory.
    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                     IID_PPV_ARGS(&pimpl->wic_factory));

    wchar_t locale_storage[LOCALE_NAME_MAX_LENGTH];
    GetUserDefaultLocaleName(locale_storage, LOCALE_NAME_MAX_LENGTH);
    pimpl->locale = locale_storage;

    ComPtr<IDWriteRenderingParams> default_params;
    pimpl->dwrite_factory->CreateRenderingParams(&default_params);
    FLOAT gamma = default_params->GetGamma();
    FLOAT enhanced_contrast = default_params->GetEnhancedContrast();
    FLOAT cleartype_level = default_params->GetClearTypeLevel();
    // TODO: See if we should hard code pixel geometry and rendering mode.
    pimpl->dwrite_factory->CreateCustomRenderingParams(
        gamma, enhanced_contrast, cleartype_level, DWRITE_PIXEL_GEOMETRY_RGB,
        DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC, &pimpl->text_rendering_params);

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
    pimpl->d2d1_factory->CreateDCRenderTarget(&render_target_properties, &pimpl->dc_target);
    pimpl->dc_target->SetTextRenderingParams(pimpl->text_rendering_params.Get());
}

FontRasterizer::~FontRasterizer() {}

FontId FontRasterizer::add_font(std::string_view font_name8, int font_size, FontStyle font_style) {
    std::wstring font_name16 = base::windows::convert_to_utf16(font_name8);

    ComPtr<IDWriteFontCollection> font_collection;
    pimpl->dwrite_factory->GetSystemFontCollection(&font_collection);

    // https://stackoverflow.com/q/40365439/14698275
    UINT32 index;
    BOOL exists;
    HRESULT hr;
    hr = font_collection->FindFamilyName(font_name16.data(), &index, &exists);
    if (FAILED(hr)) {
        spdlog::error("Could not create font family with name {} and size {}.", font_name8,
                      font_size);
        std::abort();
    }

    ComPtr<IDWriteFontFamily> font_family;
    hr = font_collection->GetFontFamily(index, &font_family);
    if (FAILED(hr)) {
        spdlog::error("Could not create font family with name {} and size {}.", font_name8,
                      font_size);
        std::abort();
    }

    auto style = DWRITE_FONT_STYLE_NORMAL;
    auto weight = DWRITE_FONT_WEIGHT_NORMAL;
    if (font_style == FontStyle::kNone) {
        style = DWRITE_FONT_STYLE_NORMAL;
    } else if (font_style == FontStyle::kBold) {
        weight = DWRITE_FONT_WEIGHT_BOLD;
    } else if (font_style == FontStyle::kItalic) {
        style = DWRITE_FONT_STYLE_ITALIC;
    }

    // TODO: Do we need this?
    // ComPtr<IDWriteFontCollection1> font_collection1;
    // font_collection.As(&font_collection1);
    // ComPtr<IDWriteFontSet> font_set;
    // font_collection1->GetFontSet(&font_set);
    // ComPtr<IDWriteFontSet> filtered_set;
    // font_set->GetMatchingFonts(font_name16.data(), weight, DWRITE_FONT_STRETCH_NORMAL, style,
    //                            &filtered_set);
    // UINT32 font_count = filtered_set->GetFontCount();
    // for (UINT32 i = 0; i < font_count; ++i) {
    //     ComPtr<IDWriteFontFaceReference> font_face_ref;
    //     filtered_set->GetFontFaceReference(i, &font_face_ref);
    //     ComPtr<IDWriteFontFace3> font_face;
    //     font_face_ref->CreateFontFace(&font_face);

    //     ComPtr<IDWriteFont> font;
    //     HRESULT hr = font_collection1->GetFontFromFontFace(font_face.Get(), &font);

    //     std::wstring font_name = GetPostscriptName(font.Get(), pimpl->locale);
    //     spdlog::info(L"font name = {}", font_name);

    //     if (SUCCEEDED(hr)) {
    //         spdlog::info("Found font in filtered collection!");
    //         return cache_font({font}, font_size);
    //     }
    // }

    ComPtr<IDWriteFont> font;
    hr = font_family->GetFirstMatchingFont(weight, DWRITE_FONT_STRETCH_NORMAL, style, &font);
    if (FAILED(hr)) {
        spdlog::error("Could not create font with name {} and size {}.", font_name8, font_size);
        std::abort();
    }

    return cache_font({font}, font_size);
}

FontId FontRasterizer::add_system_font(int font_size, FontStyle font_style) {
    NONCLIENTMETRICS metrics = {};
    metrics.cbSize = sizeof(metrics);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, metrics.cbSize, &metrics, 0);

    ComPtr<IDWriteGdiInterop> gdi_interop;
    pimpl->dwrite_factory->GetGdiInterop(&gdi_interop);
    ComPtr<IDWriteFont> sys_font;
    gdi_interop->CreateFontFromLOGFONT(&metrics.lfMessageFont, &sys_font);

    std::wstring font_name16 = GetFontFamilyName(sys_font.Get(), pimpl->locale);
    std::string font_name8 = base::windows::convert_to_utf8(font_name16);
    return add_font(font_name8, font_size, font_style);
}

FontId FontRasterizer::resize_font(FontId font_id, int font_size) {
    ComPtr<IDWriteFont> font = font_id_to_native[font_id].font;
    std::wstring font_name16 = GetFontFamilyName(font.Get(), pimpl->locale);
    std::string font_name8 = base::windows::convert_to_utf8(font_name16);
    return add_font(font_name8, font_size);
}

RasterizedGlyph FontRasterizer::rasterize(FontId font_id, uint32_t glyph_id) const {
    HRESULT hr;

    int scale_factor = 2;

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

    ComPtr<ID2D1DeviceContext4> dc_target4;
    hr = pimpl->dc_target.As(&dc_target4);
    if (FAILED(hr)) {
        spdlog::error("ID2D1DeviceContext4 error: Windows 10 is the oldest supported version");
        std::abort();
    }
    dc_target4->SetUnitMode(D2D1_UNIT_MODE_DIPS);
    dc_target4->SetDpi(96.0 * scale_factor, 96.0 * scale_factor);

    D2D1_RECT_F bounds;
    dc_target4->GetGlyphRunWorldBounds({}, &glyph_run, DWRITE_MEASURING_MODE_NATURAL, &bounds);
    // TODO: Clean this up.
    if (bounds.right < bounds.left) {
        return {};
    }

    int left = std::floor(bounds.left);
    int right = std::ceil(bounds.right);
    int top = std::floor(bounds.top);
    int bottom = std::ceil(bounds.bottom);

    int width = right - left;
    int height = bottom - top;
    // TODO: Is this ascent or descent?
    int descent = -top;

    D2D1_POINT_2F baseline_origin = {
        .x = static_cast<FLOAT>(-left),
        .y = static_cast<FLOAT>(-top),
    };

    width *= scale_factor;
    height *= scale_factor;
    left *= scale_factor;
    descent *= scale_factor;

    ComPtr<IWICBitmap> wic_bitmap;
    pimpl->wic_factory->CreateBitmap(width, height, GUID_WICPixelFormat32bppPBGRA,
                                     WICBitmapCacheOnDemand, &wic_bitmap);

    // TODO: Consider making a helper function for this. Also see if the below method works.
    // D2D1_RENDER_TARGET_PROPERTIES render_target_properties = D2D1::RenderTargetProperties();
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

    ComPtr<ID2D1RenderTarget> render_target;
    pimpl->d2d1_factory->CreateWicBitmapRenderTarget(wic_bitmap.Get(), render_target_properties,
                                                     &render_target);
    ComPtr<ID2D1DeviceContext4> render_target4;
    hr = render_target.As(&render_target4);
    if (FAILED(hr)) {
        spdlog::error("ID2D1DeviceContext4 error: Windows 10 is the oldest supported version");
        std::abort();
    }
    render_target4->SetUnitMode(D2D1_UNIT_MODE_DIPS);
    render_target4->SetDpi(96.0 * scale_factor, 96.0 * scale_factor);
    render_target4->SetTextRenderingParams(pimpl->text_rendering_params.Get());

    ComPtr<IDWriteColorGlyphRunEnumerator1> color_run_enumerator;
    DWRITE_GLYPH_IMAGE_FORMATS image_formats = DWRITE_GLYPH_IMAGE_FORMATS_COLR;
    hr = pimpl->dwrite_factory->TranslateColorGlyphRun({}, &glyph_run, nullptr, image_formats,
                                                       DWRITE_MEASURING_MODE_NATURAL, nullptr, 0,
                                                       &color_run_enumerator);

    ComPtr<ID2D1SolidColorBrush> brush;
    render_target4->CreateSolidColorBrush({1.0f, 1.0f, 1.0f, 1.0f}, &brush);

    render_target4->BeginDraw();

    bool colored = hr != DWRITE_E_NOCOLOR;
    if (colored) {
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
                render_target4->DrawGlyphRun(baseline_origin, &color_run->glyphRun, brush.Get(),
                                             color_run->measuringMode);
                break;
            default:
                spdlog::error("Error: DirectWrite glyph image format unimplemented");
                std::abort();
            }
        }
    } else {
        render_target4->DrawGlyphRun(baseline_origin, &glyph_run, brush.Get(),
                                     DWRITE_MEASURING_MODE_NATURAL);
    }
    render_target4->EndDraw();

    UINT stride = width * 4;
    std::vector<uint8_t> bitmap_data(stride * height);
    wic_bitmap->CopyPixels(nullptr, stride, bitmap_data.size(), bitmap_data.data());

    return {
        .left = left,
        .top = descent,
        .width = static_cast<int32_t>(width),
        .height = static_cast<int32_t>(height),
        .buffer = std::move(bitmap_data),
        .colored = colored,
    };
}

namespace {

// Invert cluster map (string index -> glyph index) to (glyph index -> string index).
std::vector<size_t> get_inverted_cluster_map(
    const DWRITE_GLYPH_RUN_DESCRIPTION* glyph_run_description, size_t glyph_count) {

    auto cluster_map = glyph_run_description->clusterMap;
    size_t len = glyph_run_description->stringLength;

    std::vector<size_t> inverted_cluster_map(glyph_count);
    for (size_t i = 0; i < len; ++i) {
        if (i > 0 && cluster_map[i] == cluster_map[i - 1]) {
            continue;
        }
        size_t glyph_index = cluster_map[i];
        inverted_cluster_map[glyph_index] = i;
    }
    return inverted_cluster_map;
}

class FontFallbackRenderer : public IDWriteTextRenderer {
public:
    FontFallbackRenderer(ComPtr<IDWriteFontCollection> font_collection, std::string_view str8)
        : ref_count(1), font_collection(font_collection) {

        if (!indices_map.set_utf8(str8.data(), str8.length())) {
            spdlog::error("UTF16ToUTF8IndicesMap::setUTF8 error");
            std::abort();
        }
    }

    // IUnknown methods.
    HRESULT WINAPI QueryInterface(IID const& riid, void** ppv_object) override {
        if (__uuidof(IUnknown) == riid || __uuidof(IDWritePixelSnapping) == riid ||
            __uuidof(IDWriteTextRenderer) == riid) {
            *ppv_object = this;
            this->AddRef();
            return S_OK;
        }
        *ppv_object = nullptr;
        return E_FAIL;
    }

    ULONG WINAPI AddRef() override { return InterlockedIncrement(&ref_count); }

    ULONG WINAPI Release() override {
        ULONG new_count = InterlockedDecrement(&ref_count);
        if (new_count == 0) {
            delete this;
        }
        return new_count;
    }

    // IDWriteTextRenderer methods.
    HRESULT WINAPI DrawGlyphRun(void* client_drawing_context,
                                FLOAT baseline_origin_x,
                                FLOAT baseline_origin_y,
                                DWRITE_MEASURING_MODE measuring_mode,
                                DWRITE_GLYPH_RUN const* glyph_run,
                                DWRITE_GLYPH_RUN_DESCRIPTION const* glyph_run_description,
                                IUnknown* client_drawing_effect) override {

        int scale_factor = 2;

        if (!glyph_run->fontFace) {
            spdlog::error("Glyph run without font face.");
            std::abort();
        }
        if (glyph_run->glyphCount == 0) {
            return S_OK;
        }

        // Cache font.
        auto font_rasterizer = static_cast<FontRasterizer*>(client_drawing_context);
        ComPtr<IDWriteFont> font;
        font_collection->GetFontFromFontFace(glyph_run->fontFace, &font);
        int font_size = glyph_run->fontEmSize * 72 / 96;
        size_t run_font_id = font_rasterizer->cache_font({font}, font_size);

        size_t glyph_count = glyph_run->glyphCount;
        size_t text_position = glyph_run_description->textPosition;
        auto inverted_cluster_map = get_inverted_cluster_map(glyph_run_description, glyph_count);

        for (size_t i = 0; i < glyph_count; ++i) {
            uint32_t glyph_id = glyph_run->glyphIndices[i];
            // TODO: Verify that rounding up is correct.
            int advance = std::ceil(glyph_run->glyphAdvances[i] * scale_factor);

            size_t utf8_index = indices_map.map_index(text_position + inverted_cluster_map[i]);
            ShapedGlyph glyph = {
                .font_id = run_font_id,
                .glyph_id = glyph_id,
                // TODO: Do we need the y values?
                .position = {.x = total_advance},
                .advance = {.x = advance},
                .index = utf8_index,
            };
            glyphs.emplace_back(std::move(glyph));

            total_advance += advance;
        }

        return S_OK;
    }

    HRESULT WINAPI DrawUnderline(void* client_drawing_context,
                                 FLOAT baseline_origin_x,
                                 FLOAT baseline_origin_y,
                                 DWRITE_UNDERLINE const* underline,
                                 IUnknown* client_drawing_effect) override {
        return E_NOTIMPL;
    }

    HRESULT WINAPI DrawStrikethrough(void* client_drawing_context,
                                     FLOAT baseline_origin_x,
                                     FLOAT baseline_origin_y,
                                     DWRITE_STRIKETHROUGH const* strikethrough,
                                     IUnknown* client_drawing_effect) override {
        return E_NOTIMPL;
    }

    HRESULT WINAPI DrawInlineObject(void* client_drawing_context,
                                    FLOAT origin_x,
                                    FLOAT origin_y,
                                    IDWriteInlineObject* inline_object,
                                    BOOL is_sideways,
                                    BOOL is_right_to_left,
                                    IUnknown* client_drawing_effect) override {
        return E_NOTIMPL;
    }

    // IDWritePixelSnapping methods.
    HRESULT WINAPI IsPixelSnappingDisabled(void* client_drawing_context,
                                           BOOL* is_disabled) override {
        *is_disabled = false;
        return S_OK;
    }

    HRESULT WINAPI GetCurrentTransform(void* client_drawing_context,
                                       DWRITE_MATRIX* transform) override {
        static constexpr DWRITE_MATRIX ident = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
        *transform = ident;
        return S_OK;
    }

    HRESULT WINAPI GetPixelsPerDip(void* client_drawing_context, FLOAT* pixels_per_dip) override {
        *pixels_per_dip = 1.0f;
        return S_OK;
    }

private:
    virtual ~FontFallbackRenderer() {}

    ULONG ref_count;
    ComPtr<IDWriteFontCollection> font_collection;
    unicode::UTF16ToUTF8IndicesMap indices_map;

    // Outputs for FontRasterizer.
    friend class ::font::FontRasterizer;
    int total_advance = 0;
    std::vector<ShapedGlyph> glyphs;
};

}  // namespace

LineLayout FontRasterizer::layout_line(FontId font_id, std::string_view str8) {
    DCHECK_EQ(str8.find('\n'), std::string_view::npos);

    std::wstring str16 = base::windows::convert_to_utf16(str8);

    auto& native_font = font_id_to_native[font_id];
    const auto& dwrite_info = pimpl->font_id_to_dwrite_info[font_id];
    ComPtr<IDWriteFont> font = native_font.font;

    ComPtr<IDWriteFontCollection> font_collection;
    pimpl->dwrite_factory->GetSystemFontCollection(&font_collection);

    ComPtr<IDWriteTextFormat> text_format;
    pimpl->dwrite_factory->CreateTextFormat(
        dwrite_info.font_name16.data(), font_collection.Get(), font->GetWeight(), font->GetStyle(),
        DWRITE_FONT_STRETCH_NORMAL, dwrite_info.em_size, pimpl->locale.data(), &text_format);

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

    text_layout->SetFontCollection(font_collection.Get(), {0, len});

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
    int scale_factor = 2;

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

    int ascent = std::ceil(dwrite_metrics.ascent * scale * scale_factor);
    int descent = std::ceil(dwrite_metrics.descent * scale * scale_factor);
    int line_gap = std::ceil(dwrite_metrics.lineGap * scale * scale_factor);

    int line_height = ascent + descent + line_gap;

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
    std::string font_name8 = base::windows::convert_to_utf8(font_name);
    font_id_to_postscript_name.emplace_back(std::move(font_name8));
    pimpl->font_id_to_dwrite_info.emplace_back(std::move(dwrite_info));
    return font_id;
}

namespace {

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

    wchar_t name[length + 1];
    strings->GetString(index, name, length + 1);
    return name;
}

}  // namespace

}  // namespace font
