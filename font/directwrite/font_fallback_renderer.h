#pragma once

#include "font/types.h"
#include "unicode/utf16_to_utf8_indices_map.h"

#include <dwrite_3.h>
#include <wrl/client.h>

// https://skia.googlesource.com/skia/+/0a7c7b0b96fc897040e71ea3304d9d6a042cda8b/src/utils/win/SkObjBase.h
// TODO: Define this elsewhere.
#define SK_STDMETHODIMP COM_DECLSPEC_NOTHROW STDMETHODIMP
#define SK_STDMETHODIMP_(type) COM_DECLSPEC_NOTHROW STDMETHODIMP_(type)

namespace font {

class FontFallbackRenderer : public IDWriteTextRenderer {
public:
    // TODO: Consider encapsulating these with getters.
    int total_advance = 0;
    std::vector<ShapedRun> runs;

    FontFallbackRenderer(Microsoft::WRL::ComPtr<IDWriteFontCollection> font_collection,
                         std::string_view str8);

    // IUnknown methods
    SK_STDMETHODIMP QueryInterface(IID const& riid, void** ppvObject) override;
    SK_STDMETHODIMP_(ULONG) AddRef() override;
    SK_STDMETHODIMP_(ULONG) Release() override;

    // IDWriteTextRenderer methods
    SK_STDMETHODIMP DrawGlyphRun(void* clientDrawingContext,
                                 FLOAT baselineOriginX,
                                 FLOAT baselineOriginY,
                                 DWRITE_MEASURING_MODE measuringMode,
                                 DWRITE_GLYPH_RUN const* glyphRun,
                                 DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
                                 IUnknown* clientDrawingEffect) override;
    SK_STDMETHODIMP DrawUnderline(void* clientDrawingContext,
                                  FLOAT baselineOriginX,
                                  FLOAT baselineOriginY,
                                  DWRITE_UNDERLINE const* underline,
                                  IUnknown* clientDrawingEffect) override;
    SK_STDMETHODIMP DrawStrikethrough(void* clientDrawingContext,
                                      FLOAT baselineOriginX,
                                      FLOAT baselineOriginY,
                                      DWRITE_STRIKETHROUGH const* strikethrough,
                                      IUnknown* clientDrawingEffect) override;
    SK_STDMETHODIMP DrawInlineObject(void* clientDrawingContext,
                                     FLOAT originX,
                                     FLOAT originY,
                                     IDWriteInlineObject* inlineObject,
                                     BOOL isSideways,
                                     BOOL isRightToLeft,
                                     IUnknown* clientDrawingEffect) override;

    // IDWritePixelSnapping methods
    SK_STDMETHODIMP IsPixelSnappingDisabled(void* clientDrawingContext, BOOL* isDisabled) override;
    SK_STDMETHODIMP GetCurrentTransform(void* clientDrawingContext,
                                        DWRITE_MATRIX* transform) override;
    SK_STDMETHODIMP GetPixelsPerDip(void* clientDrawingContext, FLOAT* pixelsPerDip) override;

private:
    virtual ~FontFallbackRenderer() {}

    ULONG fRefCount;
    Microsoft::WRL::ComPtr<IDWriteFontCollection> font_collection;
    UTF16ToUTF8IndicesMap utf8IndicesMap;
};

}  // namespace font
