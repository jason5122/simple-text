#include "font_fallback_renderer.h"

#include "font/directwrite/impl_directwrite.h"

using Microsoft::WRL::ComPtr;

// TODO: Debug use; remove this.
#include <fmt/base.h>

namespace font {

FontFallbackRenderer::FontFallbackRenderer(ComPtr<IDWriteFontCollection> font_collection,
                                           std::string_view str8)
    : fRefCount(1), font_collection{font_collection} {
    if (!utf8IndicesMap.setUTF8(str8.data(), str8.length())) {
        fmt::println("UTF16ToUTF8IndicesMap::setUTF8 error");
        std::abort();
    }
}

// IUnknown methods
SK_STDMETHODIMP FontFallbackRenderer::QueryInterface(IID const& riid, void** ppvObject) {
    if (__uuidof(IUnknown) == riid || __uuidof(IDWritePixelSnapping) == riid ||
        __uuidof(IDWriteTextRenderer) == riid) {
        *ppvObject = this;
        this->AddRef();
        return S_OK;
    }
    *ppvObject = nullptr;
    return E_FAIL;
}

SK_STDMETHODIMP_(ULONG) FontFallbackRenderer::AddRef() {
    return InterlockedIncrement(&fRefCount);
}

SK_STDMETHODIMP_(ULONG) FontFallbackRenderer::Release() {
    ULONG newCount = InterlockedDecrement(&fRefCount);
    if (newCount == 0) {
        delete this;
    }
    return newCount;
}

// IDWriteTextRenderer methods
SK_STDMETHODIMP FontFallbackRenderer::DrawGlyphRun(
    void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_MEASURING_MODE measuringMode,
    DWRITE_GLYPH_RUN const* glyphRun,
    DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
    IUnknown* clientDrawingEffect) {
    if (!glyphRun->fontFace) {
        fmt::println("Glyph run without font face.");
        std::abort();
    }

    // Cache font.
    ComPtr<IDWriteFont> font;
    font_collection->GetFontFromFontFace(glyphRun->fontFace, &font);
    int font_size = glyphRun->fontEmSize * 72 / 96;
    auto font_rasterizer = static_cast<FontRasterizer*>(clientDrawingContext);
    size_t font_id = font_rasterizer->cacheFont({font}, font_size);

    size_t glyph_count = glyphRun->glyphCount;
    std::vector<ShapedGlyph> glyphs;
    glyphs.reserve(glyph_count);

    auto cluster_map = glyphRunDescription->clusterMap;
    size_t text_position = glyphRunDescription->textPosition;
    size_t len = glyphRunDescription->stringLength;

    // Invert cluster map (string index -> glyph index) to (glyph index -> string index).
    std::vector<size_t> inverted_cluster_map(glyph_count);
    size_t i = 0;
    while (i < len) {
        size_t glyph_index = cluster_map[i];
        inverted_cluster_map[glyph_index] = i;

        while (i < len && cluster_map[i] == glyph_index) {
            ++i;
        }
    }

    for (size_t i = 0; i < glyph_count; ++i) {
        uint32_t glyph_id = glyphRun->glyphIndices[i];
        // TODO: Verify that rounding up is correct.
        int advance = std::ceil(glyphRun->glyphAdvances[i]);

        ShapedGlyph glyph{
            .glyph_id = glyph_id,
            .position = {.x = total_advance},
            .advance = {.x = advance},
            .index = utf8IndicesMap.mapIndex(text_position + inverted_cluster_map[i]),
        };
        glyphs.emplace_back(std::move(glyph));

        total_advance += advance;
    }

    runs.emplace_back(ShapedRun{
        .font_id = font_id,
        .glyphs = std::move(glyphs),
    });

    return S_OK;
}

SK_STDMETHODIMP FontFallbackRenderer::DrawUnderline(void* clientDrawingContext,
                                                    FLOAT baselineOriginX,
                                                    FLOAT baselineOriginY,
                                                    DWRITE_UNDERLINE const* underline,
                                                    IUnknown* clientDrawingEffect) {
    return E_NOTIMPL;
}

SK_STDMETHODIMP FontFallbackRenderer::DrawStrikethrough(void* clientDrawingContext,
                                                        FLOAT baselineOriginX,
                                                        FLOAT baselineOriginY,
                                                        DWRITE_STRIKETHROUGH const* strikethrough,
                                                        IUnknown* clientDrawingEffect) {
    return E_NOTIMPL;
}

SK_STDMETHODIMP FontFallbackRenderer::DrawInlineObject(void* clientDrawingContext,
                                                       FLOAT originX,
                                                       FLOAT originY,
                                                       IDWriteInlineObject* inlineObject,
                                                       BOOL isSideways,
                                                       BOOL isRightToLeft,
                                                       IUnknown* clientDrawingEffect) {
    return E_NOTIMPL;
}

// IDWritePixelSnapping methods
SK_STDMETHODIMP FontFallbackRenderer::IsPixelSnappingDisabled(void* clientDrawingContext,
                                                              BOOL* isDisabled) {
    *isDisabled = FALSE;
    return S_OK;
}

SK_STDMETHODIMP FontFallbackRenderer::GetCurrentTransform(void* clientDrawingContext,
                                                          DWRITE_MATRIX* transform) {
    static constexpr DWRITE_MATRIX ident = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
    *transform = ident;
    return S_OK;
}

SK_STDMETHODIMP FontFallbackRenderer::GetPixelsPerDip(void* clientDrawingContext,
                                                      FLOAT* pixelsPerDip) {
    *pixelsPerDip = 1.0f;
    return S_OK;
}

}  // namespace font
