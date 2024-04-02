#include "rasterizer.h"
#include "third_party/libgrapheme/grapheme.h"
#include <dwrite.h>
#include <iostream>

// https://stackoverflow.com/a/64471501/14698275
std::wstring to_wide(const std::string& multi) {
    std::wstring wide;
    wchar_t w;
    mbstate_t mb{};
    size_t n = 0, len = multi.length() + 1;
    while (auto res = mbrtowc(&w, multi.c_str() + n, len - n, &mb)) {
        if (res == size_t(-1) || res == size_t(-2)) {
            std::cerr << "to_wide(): invalid encoding\n";
        }

        n += res;
        wide += w;
    }
    return wide;
}

class FontRasterizer::impl {
public:
    IDWriteFactory* dwrite_factory;
    IDWriteFontFace* font_face;
    FLOAT em_size;
};

FontRasterizer::FontRasterizer() : pimpl{new impl{}} {}

bool FontRasterizer::setup(int id, std::string main_font_name, int font_size) {
    this->id = id;

    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                        reinterpret_cast<IUnknown**>(&pimpl->dwrite_factory));

    IDWriteFontCollection* font_collection;
    pimpl->dwrite_factory->GetSystemFontCollection(&font_collection);

    // https://stackoverflow.com/q/40365439/14698275
    UINT32 index;
    BOOL exists;
    font_collection->FindFamilyName(to_wide(main_font_name).c_str(), &index, &exists);

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

    FLOAT ascent = metrics.ascent * scale;
    FLOAT descent = -metrics.descent * scale;
    FLOAT line_gap = metrics.lineGap * scale;

    FLOAT line_height = ascent - descent + line_gap;

    this->line_height = line_height;
    this->descent = descent;

    return true;
}

RasterizedGlyph FontRasterizer::rasterizeUTF32(uint_least32_t codepoint) {
    // pimpl->dwrite_factory->GetSystemFontFallback();

    UINT16* glyph_indices = new UINT16[1];
    pimpl->font_face->GetGlyphIndices(&codepoint, 1, glyph_indices);

    FLOAT glyph_advances = 0;
    DWRITE_GLYPH_OFFSET offset = {0};
    DWRITE_GLYPH_RUN glyph_run{
        .fontFace = pimpl->font_face,
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

    // TODO: Experiment with CreateBitmapRenderTarget() and DrawGlyphRun().
    // https://github.com/sublimehq/sublime_text/issues/2350#issuecomment-400367215
    {
        IDWriteTextFormat* text_format;
        pimpl->dwrite_factory->CreateTextFormat(
            L"Source Code Pro", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 32, L"en-us", &text_format);

        IDWriteTextLayout* text_layout;
        // TODO: Properly set `maxWidth`/`maxHeight`.
        pimpl->dwrite_factory->CreateTextLayout(L"a", 1, text_format, 10000, 10000, &text_layout);

        IDWriteGdiInterop* gdi_interop;
        pimpl->dwrite_factory->GetGdiInterop(&gdi_interop);

        IDWriteBitmapRenderTarget* render_target;
        gdi_interop->CreateBitmapRenderTarget(nullptr, 128, 128, &render_target);

        render_target->DrawGlyphRun(0, 0, DWRITE_MEASURING_MODE_NATURAL, &glyph_run,
                                    rendering_params, RGB(0, 200, 255));

        HDC memory_hdc = render_target->GetMemoryDC();
        HBITMAP hbitmap = (HBITMAP)GetCurrentObject(memory_hdc, OBJ_BITMAP);

        BITMAPINFO bm_info = {0};
        GetDIBits(memory_hdc, hbitmap, 0, 0, nullptr, &bm_info, DIB_RGB_COLORS);

        BYTE* pixels = new BYTE[bm_info.bmiHeader.biSizeImage];
        bm_info.bmiHeader.biCompression = BI_RGB;

        GetDIBits(memory_hdc, hbitmap, 0, bm_info.bmiHeader.biHeight, (LPVOID)pixels, &bm_info,
                  DIB_RGB_COLORS);

        for (size_t i = 0; i < 100; i++) {
            std::cout << (int)pixels[i] << ' ';
        }
        std::cerr << '\n';

        // BITMAP bitmap;
        // GetObject(hbitmap, sizeof(bitmap), (LPVOID)&bitmap);

        // std::cerr << bitmap.bmWidth << "x" << bitmap.bmHeight << '\n';

        // for (size_t i = 0; i < bitmap.bmWidth * bitmap.bmHeight; i++) {
        //     std::cerr << bitmap.bmBits[i] << ' ';
        // }
        // std::cerr << '\n';
    }

    DWRITE_RENDERING_MODE rendering_mode;
    pimpl->font_face->GetRecommendedRenderingMode(
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
    BYTE* alpha_values = new BYTE[size];
    glyph_run_analysis->CreateAlphaTexture(DWRITE_TEXTURE_CLEARTYPE_3x1, &texture_bounds,
                                           alpha_values, size);

    std::vector<uint8_t> buffer;
    buffer.reserve(size);
    for (size_t i = 0; i < size; i++) {
        buffer.push_back(alpha_values[i]);
    }

    DWRITE_GLYPH_METRICS metrics;
    pimpl->font_face->GetDesignGlyphMetrics(glyph_indices, 1, &metrics, false);

    DWRITE_FONT_METRICS font_metrics;
    pimpl->font_face->GetMetrics(&font_metrics);

    FLOAT scale = pimpl->em_size / font_metrics.designUnitsPerEm;
    FLOAT advance = metrics.advanceWidth * scale;

    int32_t top = -texture_bounds.top;
    top -= descent;

    return RasterizedGlyph{
        .colored = false,
        .left = texture_bounds.left,
        .top = top,
        .width = static_cast<int32_t>(pixel_width),
        .height = static_cast<int32_t>(pixel_height),
        .advance = advance,
        .buffer = buffer,
        .index = glyph_indices[0],
    };
}

std::vector<RasterizedGlyph> FontRasterizer::layoutLine(const char* utf8_str) {
    return {};
}

FontRasterizer::~FontRasterizer() {}
