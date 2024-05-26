#include "font/directwrite/directwrite_helper.h"
#include "font/directwrite/font_fallback_source.h"
#include "font/directwrite/text_analysis.h"
#include "font/rasterizer.h"
#include <combaseapi.h>
#include <comdef.h>
#include <cwchar>
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
    ComPtr<IDWriteFontFace> font_face;
    FLOAT em_size;

    ComPtr<ID2D1Factory> d2d_factory;
    ComPtr<IWICImagingFactory2> wic_factory;
    ComPtr<IDWriteFontFallback> font_fallback_;

    ComPtr<IDWriteFontFace> mapCharacters(std::wstring_view utf16_str);
    UINT16 getGlyphIndex(std::wstring_view utf16_str, IDWriteFontFace* font_face);
};

FontRasterizer::FontRasterizer() : pimpl{new impl{}} {}

FontRasterizer::~FontRasterizer() {}

bool FontRasterizer::setup(int id, std::string font_name, int font_size) {
    this->id = id;

    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory4),
                        reinterpret_cast<IUnknown**>(pimpl->dwrite_factory.GetAddressOf()));
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, pimpl->d2d_factory.GetAddressOf());

    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pimpl->wic_factory));
    if (FAILED(hr)) {
        _com_error err(hr);
        std::cerr << err.ErrorMessage() << '\n';
    }

    pimpl->dwrite_factory->GetSystemFontFallback(&pimpl->font_fallback_);

    ComPtr<IDWriteFontCollection> font_collection;
    pimpl->dwrite_factory->GetSystemFontCollection(&font_collection);

    // https://stackoverflow.com/q/40365439/14698275
    std::wstring wstr = ConvertToUTF16(font_name);
    UINT32 index;
    BOOL exists;
    font_collection->FindFamilyName(&wstr[0], &index, &exists);

    IDWriteFontFamily* font_family;
    font_collection->GetFontFamily(index, &font_family);

    IDWriteFont* font;
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

    return true;
}

RasterizedGlyph FontRasterizer::rasterizeUTF8(std::string_view utf8_str) {
    std::wstring utf16_str = ConvertToUTF16(utf8_str);

    ComPtr<IDWriteFontFace> font_face = pimpl->font_face;
    UINT16 glyph_index = 0;

    if (glyph_index == 0) {
        ComPtr<IDWriteFontFace> mapped_font_face = pimpl->mapCharacters(utf16_str);

        // If no fallback font is found, don't do anything and leave the glyph index as 0.
        // Let the glyph be rendered as the "tofu" glyph.
        if (mapped_font_face != nullptr) {
            font_face = mapped_font_face;
            glyph_index = pimpl->getGlyphIndex(utf16_str, mapped_font_face.Get());
        }
    }

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

    IDWriteFontFace2* font_face_2;
    font_face->QueryInterface(reinterpret_cast<IDWriteFontFace2**>(&font_face_2));
    if (font_face_2->IsColorFont()) {
        DWRITE_GLYPH_IMAGE_FORMATS image_formats = DWRITE_GLYPH_IMAGE_FORMATS_COLR;
        hr = pimpl->dwrite_factory->TranslateColorGlyphRun({}, &glyph_run, nullptr, image_formats,
                                                           DWRITE_MEASURING_MODE_NATURAL, nullptr,
                                                           0, &color_run_enumerator);
    }

    if (hr == DWRITE_E_NOCOLOR) {
        std::vector<BYTE> alpha_values(size);
        glyph_run_analysis->CreateAlphaTexture(DWRITE_TEXTURE_CLEARTYPE_3x1, &texture_bounds,
                                               &alpha_values[0], size);

        std::vector<uint8_t> buffer;
        buffer.reserve(size);
        for (size_t i = 0; i < size; i++) {
            buffer.emplace_back(alpha_values[i]);
        }

        return RasterizedGlyph{
            .colored = false,
            .left = texture_bounds.left,
            .top = top,
            .width = static_cast<int32_t>(pixel_width),
            .height = static_cast<int32_t>(pixel_height),
            .advance = static_cast<int32_t>(std::ceil(advance)),
            .buffer = std::move(buffer),
        };
    } else {
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

        DrawGlyphRunHelper(target.Get(), std::move(color_run_enumerator), -texture_bounds.top);

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
        for (size_t i = 0; i < pixels; i++) {
            size_t offset = i * 4;
            temp_buffer.emplace_back(pv[offset]);
            temp_buffer.emplace_back(pv[offset + 1]);
            temp_buffer.emplace_back(pv[offset + 2]);
            temp_buffer.emplace_back(pv[offset + 3]);
        }

        bitmap_lock->Release();

        return RasterizedGlyph{
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

// https://github.com/linebender/skribo/blob/master/docs/script_matching.md#windows
// https://chromium.googlesource.com/chromium/src/+/refs/heads/main/content/browser/renderer_host/dwrite_font_proxy_impl_win.cc#389
ComPtr<IDWriteFontFace> FontRasterizer::impl::mapCharacters(std::wstring_view utf16_str) {
    ComPtr<IDWriteFont> mapped_font;

    // TODO: Don't hard code locale.
    static constexpr wchar_t locale[] = L"en-us";

    ComPtr<IDWriteNumberSubstitution> number_substitution;
    dwrite_factory->CreateNumberSubstitution(DWRITE_NUMBER_SUBSTITUTION_METHOD_NONE, locale, true,
                                             &number_substitution);

    ComPtr<IDWriteTextAnalysisSource> analysis_source = new FontFallbackSource(
        &utf16_str[0], utf16_str.length(), locale, number_substitution.Get());

    UINT32 mapped_len;
    FLOAT mapped_scale;
    font_fallback_->MapCharacters(analysis_source.Get(), 0, utf16_str.length(), nullptr, nullptr,
                                  DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                  DWRITE_FONT_STRETCH_NORMAL, &mapped_len, &mapped_font,
                                  &mapped_scale);

    if (mapped_font == nullptr) {
        return nullptr;
    }

    ComPtr<IDWriteFontFace> mapped_font_face;
    mapped_font->CreateFontFace(&mapped_font_face);
    return mapped_font_face;
}

UINT16 FontRasterizer::impl::getGlyphIndex(std::wstring_view utf16_str,
                                           IDWriteFontFace* font_face) {
    ComPtr<IDWriteTextAnalyzer> text_analyzer;
    dwrite_factory->CreateTextAnalyzer(&text_analyzer);

    TextAnalysis analysis(&utf16_str[0], utf16_str.length(), nullptr,
                          DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
    TextAnalysis::Run* run_head;
    analysis.GenerateResults(text_analyzer.Get(), &run_head);

    uint32_t max_glyph_count = 3 * utf16_str.length() / 2 + 16;

    std::vector<uint16_t> cluster_map(utf16_str.length());
    std::vector<DWRITE_SHAPING_TEXT_PROPERTIES> text_properties(utf16_str.length());
    std::vector<uint16_t> out_glyph_indices(max_glyph_count);
    std::vector<DWRITE_SHAPING_GLYPH_PROPERTIES> glyph_properties(max_glyph_count);
    uint32_t glyph_count;

    // TODO: Don't hard code locale.
    static constexpr wchar_t locale[] = L"en-us";

    // https://github.com/harfbuzz/harfbuzz/blob/2fcace77b2137abb44468a04e87d8716294641a9/src/hb-directwrite.cc#L661
    text_analyzer->GetGlyphs(&utf16_str[0], utf16_str.length(), font_face, false, false,
                             &run_head->mScript, locale, nullptr, nullptr, nullptr, 0,
                             max_glyph_count, &cluster_map[0], &text_properties[0],
                             &out_glyph_indices[0], &glyph_properties[0], &glyph_count);

    return out_glyph_indices[0];
}
}
