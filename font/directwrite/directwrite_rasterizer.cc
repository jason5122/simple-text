#include "font/directwrite/directwrite_helper.h"
#include "font/rasterizer.h"
#include <combaseapi.h>
#include <cwchar>
#include <unknwnbase.h>
#include <wincodec.h>
#include <winerror.h>
#include <wingdi.h>
#include <winnt.h>

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

namespace font {
class FontRasterizer::impl {
public:
    IDWriteFactory4* dwrite_factory;
    IDWriteFontFace* font_face;
    FLOAT em_size;

    ID2D1Factory* d2d_factory;
    ComPtr<IWICImagingFactory2> wic_factory;
};

FontRasterizer::FontRasterizer() : pimpl{new impl{}} {}

bool FontRasterizer::setup(int id, std::string main_font_name, int font_size) {
    this->id = id;

    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory4),
                        reinterpret_cast<IUnknown**>(&pimpl->dwrite_factory));
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pimpl->d2d_factory);

    // TODO: Initialize here. COM is uninitialized here, so we currently can't.
    // CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
    //                  IID_PPV_ARGS(&pimpl->wic_factory));

    IDWriteFontCollection* font_collection;
    pimpl->dwrite_factory->GetSystemFontCollection(&font_collection);

    // https://stackoverflow.com/a/6693107/14698275
    int wchars_num = MultiByteToWideChar(CP_UTF8, 0, &main_font_name[0], -1, NULL, 0);
    wchar_t* wstr = new wchar_t[wchars_num];
    MultiByteToWideChar(CP_UTF8, 0, &main_font_name[0], -1, wstr, wchars_num);

    // https://stackoverflow.com/q/40365439/14698275
    UINT32 index;
    BOOL exists;
    font_collection->FindFamilyName(wstr, &index, &exists);

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

RasterizedGlyph FontRasterizer::rasterizeTemp(std::string_view utf8_str,
                                              uint_least32_t codepoint) {
    IDWriteFontFace* selected_font_face = pimpl->font_face;

    // TODO: Consider replacing GetGlyphIndices() with TextAnalyzer approach.
    UINT16* glyph_indices = new UINT16[1];
    selected_font_face->GetGlyphIndices(&codepoint, 1, glyph_indices);

    if (glyph_indices[0] == 0) {
        GetFallbackFont(pimpl->dwrite_factory, utf8_str, &selected_font_face, glyph_indices);
    }

    FLOAT glyph_advances = 0;
    DWRITE_GLYPH_OFFSET offset = {0};
    DWRITE_GLYPH_RUN glyph_run{
        .fontFace = selected_font_face,
        .fontEmSize = pimpl->em_size,
        .glyphCount = 1,
        .glyphIndices = glyph_indices,
        .glyphAdvances = &glyph_advances,
        .glyphOffsets = &offset,
        .isSideways = 0,
        .bidiLevel = 0,
    };

    IDWriteRenderingParams* rendering_params;
    pimpl->dwrite_factory->CreateRenderingParams(&rendering_params);

    DWRITE_RENDERING_MODE rendering_mode;
    selected_font_face->GetRecommendedRenderingMode(
        pimpl->em_size, 1.0, DWRITE_MEASURING_MODE_NATURAL, rendering_params, &rendering_mode);

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
    selected_font_face->GetDesignGlyphMetrics(glyph_indices, 1, &metrics, false);

    DWRITE_FONT_METRICS font_metrics;
    selected_font_face->GetMetrics(&font_metrics);

    FLOAT scale = pimpl->em_size / font_metrics.designUnitsPerEm;
    FLOAT advance = metrics.advanceWidth * scale;

    int32_t top = -texture_bounds.top;
    top -= descent;

    // // TODO: Fully implement this!
    // if (pixel_width != 0 && pixel_height != 0) {
    //     // TODO: Move this up to setup() somehow.
    //     if (pimpl->wic_factory == nullptr) {
    //         CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
    //                          IID_PPV_ARGS(&pimpl->wic_factory));
    //     }

    //     ComPtr<IWICBitmap> wic_bitmap;
    //     // TODO: Implement without magic numbers. Properly find the right width/height.
    //     UINT bitmap_width = pixel_width + 10;
    //     UINT bitmap_height = pixel_height + 10;
    //     pimpl->wic_factory->CreateBitmap(bitmap_width, bitmap_height,
    //                                      GUID_WICPixelFormat32bppPRGBA, WICBitmapCacheOnDemand,
    //                                      wic_bitmap.GetAddressOf());

    //     D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();

    //     // TODO: Find a way to reuse render target and brushes.
    //     ComPtr<ID2D1RenderTarget> target;
    //     pimpl->d2d_factory->CreateWicBitmapRenderTarget(wic_bitmap.Get(), props,
    //                                                     target.GetAddressOf());

    //     DrawGlyphRunHelper(target.Get(), pimpl->dwrite_factory, selected_font_face, &glyph_run,
    //                        -texture_bounds.top);

    //     IWICBitmapLock* bitmap_lock;
    //     wic_bitmap.Get()->Lock(nullptr, WICBitmapLockRead, &bitmap_lock);

    //     UINT buffer_size = 0;
    //     BYTE* pv = NULL;
    //     bitmap_lock->GetDataPointer(&buffer_size, &pv);

    //     UINT bw = 0, bh = 0;
    //     bitmap_lock->GetSize(&bw, &bh);
    //     size_t pixels = bw * bh;

    //     std::vector<uint8_t> temp_buffer;
    //     temp_buffer.reserve(pixels * 4);
    //     for (size_t i = 0; i < pixels; i++) {
    //         size_t offset = i * 4;
    //         temp_buffer.emplace_back(pv[offset]);
    //         temp_buffer.emplace_back(pv[offset + 1]);
    //         temp_buffer.emplace_back(pv[offset + 2]);
    //         temp_buffer.emplace_back(pv[offset + 3]);
    //     }

    //     bitmap_lock->Release();

    //     return RasterizedGlyph{
    //         .colored = true,
    //         .left = 0,
    //         .top = top,
    //         .width = static_cast<int32_t>(bw),
    //         .height = static_cast<int32_t>(bh),
    //         .advance = static_cast<int32_t>(std::ceil(advance)),
    //         .buffer = std::move(temp_buffer),
    //         .index = glyph_indices[0],
    //     };
    // }

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
        .index = glyph_indices[0],
    };
}

FontRasterizer::~FontRasterizer() {}
}
