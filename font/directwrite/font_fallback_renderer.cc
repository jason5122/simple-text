#include "font_fallback_renderer.h"
#include <iostream>

namespace font {

FontFallbackRenderer::FontFallbackRenderer() : fRefCount(1) {}

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
    if (0 == newCount) {
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
        std::cerr << "Glyph run without font face.\n";
        std::abort();
    }

    size_t glyph_count = glyphRun->glyphCount;
    std::vector<FontRasterizer::ShapedGlyph> glyphs;
    glyphs.reserve(glyph_count);

    for (size_t i = 0; i < glyph_count; ++i) {
        uint32_t glyph_id = glyphRun->glyphIndices[i];
        // int advance = std::ceil(glyphRun->glyphAdvances[i]);
        int advance = glyphRun->glyphAdvances[i];

        FontRasterizer::ShapedGlyph glyph{
            .glyph_id = glyph_id,
            .position = {.x = total_advance},
            .advance = {.x = advance},
            .index = 0,  // TODO: Implement this.
        };
        glyphs.push_back(std::move(glyph));

        total_advance += advance;
    }

    runs.emplace_back(FontRasterizer::ShapedRun{
        .font_id = 0,
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
    const DWRITE_MATRIX ident = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
    *transform = ident;
    return S_OK;
}

SK_STDMETHODIMP FontFallbackRenderer::GetPixelsPerDip(void* clientDrawingContext,
                                                      FLOAT* pixelsPerDip) {
    *pixelsPerDip = 1.0f;
    return S_OK;
}

}
