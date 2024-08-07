#pragma once

#include "font/directwrite/directwrite_helper.h"
#include "font/font_rasterizer.h"
#include <dwrite_3.h>
#include <wrl/client.h>

// TODO: Debug use; remove this.
#include <format>
#include <iostream>

using Microsoft::WRL::ComPtr;

// https://skia.googlesource.com/skia/+/0a7c7b0b96fc897040e71ea3304d9d6a042cda8b/src/utils/win/SkObjBase.h
// TODO: Define this elsewhere.
#define SK_STDMETHODIMP COM_DECLSPEC_NOTHROW STDMETHODIMP
#define SK_STDMETHODIMP_(type) COM_DECLSPEC_NOTHROW STDMETHODIMP_(type)

namespace font {

class FontFallbackRenderer : public IDWriteTextRenderer {
public:
    int total_advance = 0;
    std::vector<FontRasterizer::ShapedRun> runs;

    FontFallbackRenderer(IDWriteFontCollection* pFontCollection)
        : fRefCount(1), fFontCollection(pFontCollection) {}

    // IUnknown methods
    SK_STDMETHODIMP QueryInterface(IID const& riid, void** ppvObject) override {
        if (__uuidof(IUnknown) == riid || __uuidof(IDWritePixelSnapping) == riid ||
            __uuidof(IDWriteTextRenderer) == riid) {
            *ppvObject = this;
            this->AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_FAIL;
    }

    SK_STDMETHODIMP_(ULONG) AddRef() override {
        return InterlockedIncrement(&fRefCount);
    }

    SK_STDMETHODIMP_(ULONG) Release() override {
        ULONG newCount = InterlockedDecrement(&fRefCount);
        if (0 == newCount) {
            delete this;
        }
        return newCount;
    }

    // IDWriteTextRenderer methods
    SK_STDMETHODIMP DrawGlyphRun(void* clientDrawingContext,
                                 FLOAT baselineOriginX,
                                 FLOAT baselineOriginY,
                                 DWRITE_MEASURING_MODE measuringMode,
                                 DWRITE_GLYPH_RUN const* glyphRun,
                                 DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
                                 IUnknown* clientDrawingEffect) override {
        if (!glyphRun->fontFace) {
            std::cerr << "Glyph run without font face.\n";
            std::abort();
        }

        // TODO: Debug use; remove this.
        // ComPtr<IDWriteFont> font;
        // fFontCollection->GetFontFromFontFace(glyphRun->fontFace, &font);
        // PrintFontFamilyName(font.Get());

        // std::cerr << std::format("glyph_count = {}\n", glyphRun->glyphCount);
        // for (UINT32 i = 0; i < glyphRun->glyphCount; i++) {
        //     std::cerr << std::format("glyph_id[{}] = {}\n", i, glyphRun->glyphIndices[i]);
        // }
        // for (UINT32 i = 0; i < glyphRun->glyphCount; i++) {
        //     int advance = std::ceil(glyphRun->glyphAdvances[i]);
        //     std::cerr << std::format("advance[{}] = {}\n", i, advance);
        // }

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

    SK_STDMETHODIMP DrawUnderline(void* clientDrawingContext,
                                  FLOAT baselineOriginX,
                                  FLOAT baselineOriginY,
                                  DWRITE_UNDERLINE const* underline,
                                  IUnknown* clientDrawingEffect) override {
        return E_NOTIMPL;
    }

    SK_STDMETHODIMP DrawStrikethrough(void* clientDrawingContext,
                                      FLOAT baselineOriginX,
                                      FLOAT baselineOriginY,
                                      DWRITE_STRIKETHROUGH const* strikethrough,
                                      IUnknown* clientDrawingEffect) override {
        return E_NOTIMPL;
    }

    SK_STDMETHODIMP DrawInlineObject(void* clientDrawingContext,
                                     FLOAT originX,
                                     FLOAT originY,
                                     IDWriteInlineObject* inlineObject,
                                     BOOL isSideways,
                                     BOOL isRightToLeft,
                                     IUnknown* clientDrawingEffect) override {
        return E_NOTIMPL;
    }

    // IDWritePixelSnapping methods
    SK_STDMETHODIMP IsPixelSnappingDisabled(void* clientDrawingContext,
                                            BOOL* isDisabled) override {
        *isDisabled = FALSE;
        return S_OK;
    }

    SK_STDMETHODIMP GetCurrentTransform(void* clientDrawingContext,
                                        DWRITE_MATRIX* transform) override {
        const DWRITE_MATRIX ident = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
        *transform = ident;
        return S_OK;
    }

    SK_STDMETHODIMP GetPixelsPerDip(void* clientDrawingContext, FLOAT* pixelsPerDip) override {
        *pixelsPerDip = 1.0f;
        return S_OK;
    }

private:
    virtual ~FontFallbackRenderer() {}

    ULONG fRefCount;
    IDWriteFontCollection* fFontCollection;
};

}
