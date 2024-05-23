#include "rasterizer.h"
#include "third_party/libgrapheme/grapheme.h"
#include <cassert>
#include <combaseapi.h>
#include <cwchar>
#include <d2d1.h>
#include <dwrite.h>
#include <dwrite_2.h>
#include <format>
#include <iostream>
#include <unknwnbase.h>
#include <winerror.h>
#include <wingdi.h>
#include <winnt.h>

#include <Windows.Foundation.h>
#include <wrl\client.h>
#include <wrl\wrappers\corewrappers.h>

using Microsoft::WRL::ComPtr;

#include <wincodec.h>

namespace font {
class FontRasterizer::impl {
public:
    IDWriteFactory2* dwrite_factory;
    IDWriteFontFace* font_face;
    FLOAT em_size;

    ID2D1Factory* d2d_factory;
    ComPtr<IWICImagingFactory2> wic_factory;
};

FontRasterizer::FontRasterizer() : pimpl{new impl{}} {}

bool FontRasterizer::setup(int id, std::string main_font_name, int font_size) {
    this->id = id;

    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory2),
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

    FLOAT ascent = metrics.ascent * scale;
    FLOAT descent = -metrics.descent * scale;
    FLOAT line_gap = metrics.lineGap * scale;

    FLOAT line_height = ascent - descent + line_gap;

    this->line_height = line_height;
    this->descent = descent;

    return true;
}

// https://github.com/google/skia/blob/5fdc2b47dfa448b745545e897f3a70a238edf6d7/src/ports/SkFontMgr_win_dw.cpp#L356
class FontFallbackSource : public IDWriteTextAnalysisSource {
public:
    FontFallbackSource(const WCHAR* string, UINT32 length, const WCHAR* locale,
                       IDWriteNumberSubstitution* numberSubstitution)
        : fRefCount(1), fString(string), fLength(length), fLocale(locale),
          fNumberSubstitution(numberSubstitution) {}

    // IUnknown methods
    COM_DECLSPEC_NOTHROW STDMETHODIMP QueryInterface(IID const& riid, void** ppvObject) override {
        if (__uuidof(IUnknown) == riid || __uuidof(IDWriteTextAnalysisSource) == riid) {
            *ppvObject = this;
            this->AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_FAIL;
    }

    COM_DECLSPEC_NOTHROW STDMETHODIMP_(ULONG) AddRef() override {
        return InterlockedIncrement(&fRefCount);
    }

    COM_DECLSPEC_NOTHROW STDMETHODIMP_(ULONG) Release() override {
        ULONG newCount = InterlockedDecrement(&fRefCount);
        if (0 == newCount) {
            delete this;
        }
        return newCount;
    }

    // IDWriteTextAnalysisSource methods
    COM_DECLSPEC_NOTHROW STDMETHODIMP GetTextAtPosition(UINT32 textPosition,
                                                        WCHAR const** textString,
                                                        UINT32* textLength) override {
        if (fLength <= textPosition) {
            *textString = nullptr;
            *textLength = 0;
            return S_OK;
        }
        *textString = fString + textPosition;
        *textLength = fLength - textPosition;
        return S_OK;
    }

    COM_DECLSPEC_NOTHROW STDMETHODIMP GetTextBeforePosition(UINT32 textPosition,
                                                            WCHAR const** textString,
                                                            UINT32* textLength) override {
        if (textPosition < 1 || fLength <= textPosition) {
            *textString = nullptr;
            *textLength = 0;
            return S_OK;
        }
        *textString = fString;
        *textLength = textPosition;
        return S_OK;
    }

    COM_DECLSPEC_NOTHROW STDMETHODIMP_(DWRITE_READING_DIRECTION)
        GetParagraphReadingDirection() override {
        // TODO: this is also interesting.
        return DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
    }

    COM_DECLSPEC_NOTHROW STDMETHODIMP GetLocaleName(UINT32 textPosition, UINT32* textLength,
                                                    WCHAR const** localeName) override {
        *localeName = fLocale;
        return S_OK;
    }

    COM_DECLSPEC_NOTHROW STDMETHODIMP
    GetNumberSubstitution(UINT32 textPosition, UINT32* textLength,
                          IDWriteNumberSubstitution** numberSubstitution) override {
        *numberSubstitution = fNumberSubstitution;
        return S_OK;
    }

private:
    virtual ~FontFallbackSource() {}

    ULONG fRefCount;
    const WCHAR* fString;
    UINT32 fLength;
    const WCHAR* fLocale;
    IDWriteNumberSubstitution* fNumberSubstitution;
};

// https://github.com/harfbuzz/harfbuzz/blob/2fcace77b2137abb44468a04e87d8716294641a9/src/hb-directwrite.cc#L283
class TextAnalysis : public IDWriteTextAnalysisSource, public IDWriteTextAnalysisSink {
public:
    IFACEMETHOD(QueryInterface)(IID const& iid, OUT void** ppObject) {
        return S_OK;
    }
    IFACEMETHOD_(ULONG, AddRef)() {
        return 1;
    }
    IFACEMETHOD_(ULONG, Release)() {
        return 1;
    }

    // A single contiguous run of characters containing the same analysis
    // results.
    struct Run {
        uint32_t mTextStart;   // starting text position of this run
        uint32_t mTextLength;  // number of contiguous code units covered
        uint32_t mGlyphStart;  // starting glyph in the glyphs array
        uint32_t mGlyphCount;  // number of glyphs associated with this run
        // text
        DWRITE_SCRIPT_ANALYSIS mScript;
        uint8_t mBidiLevel;
        bool mIsSideways;

        bool ContainsTextPosition(uint32_t aTextPosition) const {
            return aTextPosition >= mTextStart && aTextPosition < mTextStart + mTextLength;
        }

        Run* nextRun;
    };

public:
    TextAnalysis(const wchar_t* text, uint32_t textLength, const wchar_t* localeName,
                 DWRITE_READING_DIRECTION readingDirection)
        : mTextLength(textLength), mText(text), mLocaleName(localeName),
          mReadingDirection(readingDirection), mCurrentRun(nullptr) {}
    ~TextAnalysis() {
        // delete runs, except mRunHead which is part of the TextAnalysis object
        for (Run* run = mRunHead.nextRun; run;) {
            Run* origRun = run;
            run = run->nextRun;
            delete origRun;
        }
    }

    STDMETHODIMP
    GenerateResults(IDWriteTextAnalyzer* textAnalyzer, Run** runHead) {
        // Analyzes the text using the script analyzer and returns
        // the result as a series of runs.

        HRESULT hr = S_OK;

        // Initially start out with one result that covers the entire range.
        // This result will be subdivided by the analysis processes.
        mRunHead.mTextStart = 0;
        mRunHead.mTextLength = mTextLength;
        mRunHead.mBidiLevel = (mReadingDirection == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT);
        mRunHead.nextRun = nullptr;
        mCurrentRun = &mRunHead;

        // Call each of the analyzers in sequence, recording their results.
        if (SUCCEEDED(hr = textAnalyzer->AnalyzeScript(this, 0, mTextLength, this)))
            *runHead = &mRunHead;

        return hr;
    }

    // IDWriteTextAnalysisSource implementation

    IFACEMETHODIMP
    GetTextAtPosition(uint32_t textPosition, OUT wchar_t const** textString,
                      OUT uint32_t* textLength) {
        if (textPosition >= mTextLength) {
            // No text at this position, valid query though.
            *textString = nullptr;
            *textLength = 0;
        } else {
            *textString = mText + textPosition;
            *textLength = mTextLength - textPosition;
        }
        return S_OK;
    }

    IFACEMETHODIMP
    GetTextBeforePosition(uint32_t textPosition, OUT wchar_t const** textString,
                          OUT uint32_t* textLength) {
        if (textPosition == 0 || textPosition > mTextLength) {
            // Either there is no text before here (== 0), or this
            // is an invalid position. The query is considered valid though.
            *textString = nullptr;
            *textLength = 0;
        } else {
            *textString = mText;
            *textLength = textPosition;
        }
        return S_OK;
    }

    IFACEMETHODIMP_(DWRITE_READING_DIRECTION)
    GetParagraphReadingDirection() {
        return mReadingDirection;
    }

    IFACEMETHODIMP GetLocaleName(uint32_t textPosition, uint32_t* textLength,
                                 wchar_t const** localeName) {
        return S_OK;
    }

    IFACEMETHODIMP
    GetNumberSubstitution(uint32_t textPosition, OUT uint32_t* textLength,
                          OUT IDWriteNumberSubstitution** numberSubstitution) {
        // We do not support number substitution.
        *numberSubstitution = nullptr;
        *textLength = mTextLength - textPosition;

        return S_OK;
    }

    // IDWriteTextAnalysisSink implementation

    IFACEMETHODIMP
    SetScriptAnalysis(uint32_t textPosition, uint32_t textLength,
                      DWRITE_SCRIPT_ANALYSIS const* scriptAnalysis) {
        SetCurrentRun(textPosition);
        SplitCurrentRun(textPosition);
        while (textLength > 0) {
            Run* run = FetchNextRun(&textLength);
            run->mScript = *scriptAnalysis;
        }

        return S_OK;
    }

    IFACEMETHODIMP
    SetLineBreakpoints(uint32_t textPosition, uint32_t textLength,
                       const DWRITE_LINE_BREAKPOINT* lineBreakpoints) {
        return S_OK;
    }

    IFACEMETHODIMP SetBidiLevel(uint32_t textPosition, uint32_t textLength, uint8_t explicitLevel,
                                uint8_t resolvedLevel) {
        return S_OK;
    }

    IFACEMETHODIMP
    SetNumberSubstitution(uint32_t textPosition, uint32_t textLength,
                          IDWriteNumberSubstitution* numberSubstitution) {
        return S_OK;
    }

protected:
    Run* FetchNextRun(IN OUT uint32_t* textLength) {
        // Used by the sink setters, this returns a reference to the next run.
        // Position and length are adjusted to now point after the current run
        // being returned.

        Run* origRun = mCurrentRun;
        // Split the tail if needed (the length remaining is less than the
        // current run's size).
        if (*textLength < mCurrentRun->mTextLength)
            SplitCurrentRun(mCurrentRun->mTextStart + *textLength);
        else
            // Just advance the current run.
            mCurrentRun = mCurrentRun->nextRun;
        *textLength -= origRun->mTextLength;

        // Return a reference to the run that was just current.
        return origRun;
    }

    void SetCurrentRun(uint32_t textPosition) {
        // Move the current run to the given position.
        // Since the analyzers generally return results in a forward manner,
        // this will usually just return early. If not, find the
        // corresponding run for the text position.

        if (mCurrentRun && mCurrentRun->ContainsTextPosition(textPosition)) return;

        for (Run* run = &mRunHead; run; run = run->nextRun)
            if (run->ContainsTextPosition(textPosition)) {
                mCurrentRun = run;
                return;
            }
        assert(0);  // We should always be able to find the text position in one of our runs
    }

    void SplitCurrentRun(uint32_t splitPosition) {
        if (!mCurrentRun) {
            assert(0);  // SplitCurrentRun called without current run
            // Shouldn't be calling this when no current run is set!
            return;
        }
        // Split the current run.
        if (splitPosition <= mCurrentRun->mTextStart) {
            // No need to split, already the start of a run
            // or before it. Usually the first.
            return;
        }
        Run* newRun = new Run;

        *newRun = *mCurrentRun;

        // Insert the new run in our linked list.
        newRun->nextRun = mCurrentRun->nextRun;
        mCurrentRun->nextRun = newRun;

        // Adjust runs' text positions and lengths.
        uint32_t splitPoint = splitPosition - mCurrentRun->mTextStart;
        newRun->mTextStart += splitPoint;
        newRun->mTextLength -= splitPoint;
        mCurrentRun->mTextLength = splitPoint;
        mCurrentRun = newRun;
    }

protected:
    // Input
    // (weak references are fine here, since this class is a transient
    //  stack-based helper that doesn't need to copy data)
    uint32_t mTextLength;
    const wchar_t* mText;
    const wchar_t* mLocaleName;
    DWRITE_READING_DIRECTION mReadingDirection;

    // Current processing state.
    Run* mCurrentRun;

    // Output is a list of runs starting here
    Run mRunHead;
};

static inline void DrawGlyphRun(ID2D1RenderTarget* target, IDWriteFactory2* factory2,
                                IDWriteFontFace* fontFace, DWRITE_GLYPH_RUN* glyphRun) {
    bool isColor = false;
    IDWriteColorGlyphRunEnumerator* colorLayer;

    IDWriteFontFace2* fontFace2;
    fontFace->QueryInterface(reinterpret_cast<IDWriteFontFace2**>(&fontFace2));
    if (fontFace2->IsColorFont()) {
        if (SUCCEEDED(factory2->TranslateColorGlyphRun(0, 0, glyphRun, nullptr,
                                                       DWRITE_MEASURING_MODE_NATURAL, nullptr, 0,
                                                       &colorLayer))) {
            isColor = true;
        }
    }

    ID2D1SolidColorBrush* black_brush = nullptr;
    // target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black, 1.0f), &black_brush);
    target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Blue, 1.0f), &black_brush);

    target->BeginDraw();
    target->FillRectangle(D2D1::RectF(0, 0, 300, 300), black_brush);
    target->EndDraw();

    D2D1_POINT_2F baseline_origin{};
    if (isColor) {
        BOOL hasRun;
        const DWRITE_COLOR_GLYPH_RUN* colorRun;

        while (true) {
            if (FAILED(colorLayer->MoveNext(&hasRun)) || !hasRun) {
                break;
            }
            if (FAILED(colorLayer->GetCurrentRun(&colorRun))) {
                break;
            }

            ID2D1SolidColorBrush* brush = nullptr;
            const DWRITE_COLOR_F& color = colorRun->runColor;
            if (color.r == 0.0 && color.g == 0.0 && color.b == 0.0 && color.a == 0.0) {
                target->CreateSolidColorBrush(colorRun->runColor, &brush);
            }

            target->DrawGlyphRun(baseline_origin, glyphRun, brush ? brush : black_brush);

            if (brush) {
                brush->Release();
            }
        }
    } else {
        target->DrawGlyphRun(baseline_origin, glyphRun, black_brush);
    }
}

RasterizedGlyph FontRasterizer::rasterizeTemp(std::string_view utf8_str,
                                              uint_least32_t codepoint) {
    IDWriteFontFace* selected_font_face = pimpl->font_face;

    UINT16* glyph_indices = new UINT16[1];
    selected_font_face->GetGlyphIndices(&codepoint, 1, glyph_indices);

    // https://github.com/linebender/skribo/blob/master/docs/script_matching.md#windows
    if (glyph_indices[0] == 0) {
        // https://stackoverflow.com/a/6693107/14698275
        size_t len = utf8_str.length();
        int wchars_num = MultiByteToWideChar(CP_UTF8, 0, &utf8_str[0], len, NULL, 0);

        // TODO: Use std::wstring to prevent manual memory management!
        wchar_t* wstr = new wchar_t[wchars_num];
        MultiByteToWideChar(CP_UTF8, 0, &utf8_str[0], len, wstr, wchars_num);

        wchar_t locale[] = L"en-us";

        IDWriteNumberSubstitution* number_substitution;
        pimpl->dwrite_factory->CreateNumberSubstitution(DWRITE_NUMBER_SUBSTITUTION_METHOD_NONE,
                                                        locale, true, &number_substitution);

        IDWriteTextAnalysisSource* text_analysis;
        text_analysis = new FontFallbackSource(wstr, wchars_num, locale, number_substitution);

        IDWriteFontFallback* fallback;
        pimpl->dwrite_factory->GetSystemFontFallback(&fallback);

        UINT32 mapped_len;
        IDWriteFont* mapped_font;
        FLOAT mapped_scale;
        fallback->MapCharacters(text_analysis, 0, wchars_num, nullptr, nullptr,
                                DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                DWRITE_FONT_STRETCH_NORMAL, &mapped_len, &mapped_font,
                                &mapped_scale);

        if (mapped_font != nullptr) {
            // TODO: Everything below is for debugging; remove this.
            IDWriteFontFamily* font_family;
            mapped_font->GetFontFamily(&font_family);

            IDWriteLocalizedStrings* family_names;
            font_family->GetFamilyNames(&family_names);

            wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
            int defaultLocaleSuccess =
                GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH);

            UINT32 index = 0;
            BOOL exists = false;
            family_names->FindLocaleName(localeName, &index, &exists);

            UINT32 length = 0;
            family_names->GetStringLength(index, &length);

            // TODO: Use std::wstring to prevent manual memory management!
            wchar_t* name = new (std::nothrow) wchar_t[length + 1];
            family_names->GetString(index, name, length + 1);

            // fwprintf(stderr, L"%s, wstr: %s, mapped_len: %d\n", name, wstr, mapped_len);
            delete[] name;

            IDWriteFontFace* fallback_font_face;
            mapped_font->CreateFontFace(&fallback_font_face);
            selected_font_face = fallback_font_face;
            // selected_font_face->GetGlyphIndices(&codepoint, 1, glyph_indices);

            // TODO: Fully replace above GetGlyphIndices() with this text analyzer implementation.
            IDWriteTextAnalyzer* text_analyzer;
            pimpl->dwrite_factory->CreateTextAnalyzer(&text_analyzer);

            TextAnalysis analysis(wstr, wchars_num, nullptr,
                                  DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
            TextAnalysis::Run* run_head;
            analysis.GenerateResults(text_analyzer, &run_head);

            uint32_t max_glyph_count = 3 * wchars_num / 2 + 16;

            uint16_t* cluster_map;
            cluster_map = new uint16_t[wchars_num];
            DWRITE_SHAPING_TEXT_PROPERTIES* text_properties;
            text_properties = new DWRITE_SHAPING_TEXT_PROPERTIES[wchars_num];

            uint16_t* out_glyph_indices = new uint16_t[max_glyph_count];
            DWRITE_SHAPING_GLYPH_PROPERTIES* glyph_properties;
            glyph_properties = new DWRITE_SHAPING_GLYPH_PROPERTIES[max_glyph_count];
            uint32_t glyph_count;

            // https://github.com/harfbuzz/harfbuzz/blob/2fcace77b2137abb44468a04e87d8716294641a9/src/hb-directwrite.cc#L661
            text_analyzer->GetGlyphs(wstr, wchars_num, selected_font_face, false, false,
                                     &run_head->mScript, locale, nullptr, nullptr, nullptr, 0,
                                     max_glyph_count, cluster_map, text_properties,
                                     out_glyph_indices, glyph_properties, &glyph_count);

            // std::cerr << out_glyph_indices[0] << '\n';

            glyph_indices[0] = out_glyph_indices[0];
        } else {
            // If no fallback font is found, don't do anything and leave the glyph index as 0.
            // Let the glyph be rendered as the "tofu" glyph.
            std::cerr
                << "IDWriteFontFallback::MapCharacters() error: No font can render the text.\n";
        }
        delete[] wstr;
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

    // TODO: Move this up to setup() somehow.
    if (pimpl->wic_factory == nullptr) {
        CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                         IID_PPV_ARGS(&pimpl->wic_factory));
    }

    ComPtr<IWICBitmap> wic_bitmap;
    UINT bitmap_width = 40;
    UINT bitmap_height = 40;
    pimpl->wic_factory->CreateBitmap(bitmap_width, bitmap_height, GUID_WICPixelFormat32bppPRGBA,
                                     WICBitmapCacheOnDemand, wic_bitmap.GetAddressOf());

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();
    // props.dpiX = 96;
    // props.dpiY = 96;
    // props.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
    // props.pixelFormat =
    //     D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
    // props.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    // props.usage = D2D1_RENDER_TARGET_USAGE_NONE;

    ComPtr<ID2D1RenderTarget> target;
    pimpl->d2d_factory->CreateWicBitmapRenderTarget(wic_bitmap.Get(), props,
                                                    target.GetAddressOf());

    DrawGlyphRun(target.Get(), pimpl->dwrite_factory, selected_font_face, &glyph_run);

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
        .left = static_cast<int32_t>(0),
        .top = static_cast<int32_t>(bh + descent),
        .width = static_cast<int32_t>(bw),
        .height = static_cast<int32_t>(bh),
        .advance = static_cast<int32_t>(bw),
        .buffer = std::move(temp_buffer),
        .index = 0,
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

    // TODO: Use std::vector instead to prevent memory leak!
    BYTE* alpha_values = new BYTE[size];
    glyph_run_analysis->CreateAlphaTexture(DWRITE_TEXTURE_CLEARTYPE_3x1, &texture_bounds,
                                           alpha_values, size);

    std::vector<uint8_t> buffer;
    buffer.reserve(size);
    for (size_t i = 0; i < size; i++) {
        buffer.emplace_back(alpha_values[i]);
    }

    delete[] alpha_values;

    DWRITE_GLYPH_METRICS metrics;
    selected_font_face->GetDesignGlyphMetrics(glyph_indices, 1, &metrics, false);

    DWRITE_FONT_METRICS font_metrics;
    selected_font_face->GetMetrics(&font_metrics);

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
        .advance = static_cast<int32_t>(std::ceil(advance)),
        .buffer = std::move(buffer),
        .index = glyph_indices[0],
    };
}

FontRasterizer::~FontRasterizer() {}
}
