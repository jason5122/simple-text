#pragma once

#include <dwrite_3.h>

namespace font {

// https://github.com/google/skia/blob/5fdc2b47dfa448b745545e897f3a70a238edf6d7/src/ports/SkFontMgr_win_dw.cpp#L356
class FontFallbackSource : public IDWriteTextAnalysisSource {
public:
    FontFallbackSource(const WCHAR* string, UINT32 length, const WCHAR* locale,
                       IDWriteNumberSubstitution* numberSubstitution);

    // IUnknown methods
    COM_DECLSPEC_NOTHROW STDMETHODIMP QueryInterface(IID const& riid, void** ppvObject) override;
    COM_DECLSPEC_NOTHROW STDMETHODIMP_(ULONG) AddRef() override;
    COM_DECLSPEC_NOTHROW STDMETHODIMP_(ULONG) Release() override;

    // IDWriteTextAnalysisSource methods
    COM_DECLSPEC_NOTHROW STDMETHODIMP GetTextAtPosition(UINT32 textPosition,
                                                        WCHAR const** textString,
                                                        UINT32* textLength) override;
    COM_DECLSPEC_NOTHROW STDMETHODIMP GetTextBeforePosition(UINT32 textPosition,
                                                            WCHAR const** textString,
                                                            UINT32* textLength) override;
    COM_DECLSPEC_NOTHROW STDMETHODIMP_(DWRITE_READING_DIRECTION)
        GetParagraphReadingDirection() override;
    COM_DECLSPEC_NOTHROW STDMETHODIMP GetLocaleName(UINT32 textPosition, UINT32* textLength,
                                                    WCHAR const** localeName) override;
    COM_DECLSPEC_NOTHROW STDMETHODIMP
    GetNumberSubstitution(UINT32 textPosition, UINT32* textLength,
                          IDWriteNumberSubstitution** numberSubstitution) override;

private:
    virtual ~FontFallbackSource() {}

    ULONG fRefCount;
    const WCHAR* fString;
    UINT32 fLength;
    const WCHAR* fLocale;
    IDWriteNumberSubstitution* fNumberSubstitution;
};

}
