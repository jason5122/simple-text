#include "font_fallback_source.h"

namespace font {

FontFallbackSource::FontFallbackSource(const WCHAR* string,
                                       UINT32 length,
                                       const WCHAR* locale,
                                       IDWriteNumberSubstitution* numberSubstitution)
    : fRefCount(1),
      fString(string),
      fLength(length),
      fLocale(locale),
      fNumberSubstitution(numberSubstitution) {}

COM_DECLSPEC_NOTHROW STDMETHODIMP FontFallbackSource::QueryInterface(IID const& riid,
                                                                     void** ppvObject) {
    if (__uuidof(IUnknown) == riid || __uuidof(IDWriteTextAnalysisSource) == riid) {
        *ppvObject = this;
        this->AddRef();
        return S_OK;
    }
    *ppvObject = nullptr;
    return E_FAIL;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP_(ULONG) FontFallbackSource::AddRef() {
    return InterlockedIncrement(&fRefCount);
}

COM_DECLSPEC_NOTHROW STDMETHODIMP_(ULONG) FontFallbackSource::Release() {
    ULONG newCount = InterlockedDecrement(&fRefCount);
    if (0 == newCount) {
        delete this;
    }
    return newCount;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP FontFallbackSource::GetTextAtPosition(UINT32 textPosition,
                                                                        WCHAR const** textString,
                                                                        UINT32* textLength) {
    if (fLength <= textPosition) {
        *textString = nullptr;
        *textLength = 0;
        return S_OK;
    }
    *textString = fString + textPosition;
    *textLength = fLength - textPosition;
    return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP FontFallbackSource::GetTextBeforePosition(
    UINT32 textPosition, WCHAR const** textString, UINT32* textLength) {
    if (textPosition < 1 || fLength <= textPosition) {
        *textString = nullptr;
        *textLength = 0;
        return S_OK;
    }
    *textString = fString;
    *textLength = textPosition;
    return S_OK;
}

COM_DECLSPEC_NOTHROW
STDMETHODIMP_(DWRITE_READING_DIRECTION) FontFallbackSource::GetParagraphReadingDirection() {
    // TODO: this is also interesting.
    return DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP FontFallbackSource::GetLocaleName(UINT32 textPosition,
                                                                    UINT32* textLength,
                                                                    WCHAR const** localeName) {
    *localeName = fLocale;
    return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP FontFallbackSource::GetNumberSubstitution(
    UINT32 textPosition, UINT32* textLength, IDWriteNumberSubstitution** numberSubstitution) {
    *numberSubstitution = fNumberSubstitution;
    return S_OK;
}

}
