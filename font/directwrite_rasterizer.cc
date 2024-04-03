#include "rasterizer.h"
#include "third_party/libgrapheme/grapheme.h"
#include <combaseapi.h>
#include <cwchar>
#include <dwrite_2.h>
#include <iostream>
#include <unknwnbase.h>
#include <winerror.h>
#include <wingdi.h>
#include <winnt.h>

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
    IDWriteFactory2* dwrite_factory;
    IDWriteFontFace* font_face;
    FLOAT em_size;
};

FontRasterizer::FontRasterizer() : pimpl{new impl{}} {}

bool FontRasterizer::setup(int id, std::string main_font_name, int font_size) {
    this->id = id;

    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory2),
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

// class FallbackTextAnalysisSource : public IDWriteTextAnalysisSource {
//     LONG _cRef = 1;

//     bool rtl = false;
//     Char16String string;
//     Char16String locale;
//     IDWriteNumberSubstitution* n_sub = nullptr;

// public:
//     HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID** ppvInterface) override {
//         if (IID_IUnknown == riid) {
//             AddRef();
//             *ppvInterface = (IUnknown*)this;
//         } else if (__uuidof(IMMNotificationClient) == riid) {
//             AddRef();
//             *ppvInterface = (IMMNotificationClient*)this;
//         } else {
//             *ppvInterface = nullptr;
//             return E_NOINTERFACE;
//         }
//         return S_OK;
//     }

//     ULONG STDMETHODCALLTYPE AddRef() override {
//         return InterlockedIncrement(&_cRef);
//     }

//     ULONG STDMETHODCALLTYPE Release() override {
//         ULONG ulRef = InterlockedDecrement(&_cRef);
//         if (0 == ulRef) {
//             delete this;
//         }
//         return ulRef;
//     }

//     HRESULT STDMETHODCALLTYPE GetTextAtPosition(UINT32 p_text_position,
//                                                 WCHAR const** r_text_string,
//                                                 UINT32* r_text_length) override {
//         if (p_text_position >= (UINT32)string.length()) {
//             *r_text_string = nullptr;
//             *r_text_length = 0;
//             return S_OK;
//         }
//         *r_text_string = reinterpret_cast<const wchar_t*>(string.get_data()) + p_text_position;
//         *r_text_length = string.length() - p_text_position;
//         return S_OK;
//     }

//     HRESULT STDMETHODCALLTYPE GetTextBeforePosition(UINT32 p_text_position,
//                                                     WCHAR const** r_text_string,
//                                                     UINT32* r_text_length) override {
//         if (p_text_position < 1 || p_text_position >= (UINT32)string.length()) {
//             *r_text_string = nullptr;
//             *r_text_length = 0;
//             return S_OK;
//         }
//         *r_text_string = reinterpret_cast<const wchar_t*>(string.get_data());
//         *r_text_length = p_text_position;
//         return S_OK;
//     }

//     DWRITE_READING_DIRECTION STDMETHODCALLTYPE GetParagraphReadingDirection() override {
//         return (rtl) ? DWRITE_READING_DIRECTION_RIGHT_TO_LEFT
//                      : DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
//     }

//     HRESULT STDMETHODCALLTYPE GetLocaleName(UINT32 p_text_position, UINT32* r_text_length,
//                                             WCHAR const** r_locale_name) override {
//         *r_locale_name = reinterpret_cast<const wchar_t*>(locale.get_data());
//         return S_OK;
//     }

//     HRESULT STDMETHODCALLTYPE
//     GetNumberSubstitution(UINT32 p_text_position, UINT32* r_text_length,
//                           IDWriteNumberSubstitution** r_number_substitution) override {
//         *r_number_substitution = n_sub;
//         return S_OK;
//     }

//     FallbackTextAnalysisSource(const Char16String& p_text, const Char16String& p_locale,
//                                bool p_rtl, IDWriteNumberSubstitution* p_nsub) {
//         _cRef = 1;
//         string = p_text;
//         locale = p_locale;
//         n_sub = p_nsub;
//         rtl = p_rtl;
//     };

//     virtual ~FallbackTextAnalysisSource() {}
// };

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

RasterizedGlyph FontRasterizer::rasterizeUTF32(uint_least32_t codepoint) {
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

    // TODO: Experiment with IDWriteFontFallback for mapping characters to fonts.
    //       This requires learning about Windows COM.
    // https://github.com/linebender/skribo/blob/master/docs/script_matching.md#windows
    // https://chromium.googlesource.com/chromium/src/+/c1690d39c1e6875377f4685b53a5423cb69e2947/ui/gfx/win/text_analysis_source.cc
    // https://chromium.googlesource.com/chromium/src/+/c1690d39c1e6875377f4685b53a5423cb69e2947/ui/gfx/win/text_analysis_source.h
    {
        IDWriteNumberSubstitution* number_substitution;
        pimpl->dwrite_factory->CreateNumberSubstitution(DWRITE_NUMBER_SUBSTITUTION_METHOD_NONE,
                                                        L"en-us", true, &number_substitution);

        IDWriteTextAnalysisSource* text_analysis;
        text_analysis = new FontFallbackSource(L"a", 1, L"en-us", number_substitution);

        IDWriteFontFallback* fallback;
        pimpl->dwrite_factory->GetSystemFontFallback(&fallback);

        std::wstring wstr = L"a";
        UINT32 mapped_len;
        IDWriteFont* mapped_font;
        FLOAT mapped_scale;
        fallback->MapCharacters(text_analysis, 0, wstr.length(), nullptr, nullptr,
                                DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                DWRITE_FONT_STRETCH_NORMAL, &mapped_len, &mapped_font,
                                &mapped_scale);

        // TODO: Everything below is for debugging; remove this.
        std::cerr << "mapped_len: " << mapped_len << '\n';

        IDWriteFontFamily* font_family;
        mapped_font->GetFontFamily(&font_family);

        IDWriteLocalizedStrings* family_names;
        font_family->GetFamilyNames(&family_names);

        wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
        int defaultLocaleSuccess = GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH);

        UINT32 index = 0;
        BOOL exists = false;
        family_names->FindLocaleName(localeName, &index, &exists);

        UINT32 length = 0;
        family_names->GetStringLength(index, &length);

        wchar_t* name = new (std::nothrow) wchar_t[length + 1];
        family_names->GetString(index, name, length + 1);

        fwprintf(stderr, L"%s\n", name);
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
