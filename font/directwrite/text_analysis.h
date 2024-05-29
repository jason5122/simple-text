#pragma once

#include "util/not_copyable_or_movable.h"
#include <cstdint>
#include <dwrite_3.h>

namespace font {

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
    NOT_COPYABLE(TextAnalysis)
    NOT_MOVABLE(TextAnalysis)
    TextAnalysis(const wchar_t* text, uint32_t textLength, const wchar_t* localeName,
                 DWRITE_READING_DIRECTION readingDirection);
    ~TextAnalysis();

    STDMETHODIMP
    GenerateResults(IDWriteTextAnalyzer* textAnalyzer, Run** runHead);

    // IDWriteTextAnalysisSource implementation
    IFACEMETHODIMP
    GetTextAtPosition(uint32_t textPosition, OUT wchar_t const** textString,
                      OUT uint32_t* textLength);
    IFACEMETHODIMP
    GetTextBeforePosition(uint32_t textPosition, OUT wchar_t const** textString,
                          OUT uint32_t* textLength);
    IFACEMETHODIMP_(DWRITE_READING_DIRECTION)
    GetParagraphReadingDirection();
    IFACEMETHODIMP GetLocaleName(uint32_t textPosition, uint32_t* textLength,
                                 wchar_t const** localeName);
    IFACEMETHODIMP
    GetNumberSubstitution(uint32_t textPosition, OUT uint32_t* textLength,
                          OUT IDWriteNumberSubstitution** numberSubstitution);

    // IDWriteTextAnalysisSink implementation
    IFACEMETHODIMP
    SetScriptAnalysis(uint32_t textPosition, uint32_t textLength,
                      DWRITE_SCRIPT_ANALYSIS const* scriptAnalysis);
    IFACEMETHODIMP
    SetLineBreakpoints(uint32_t textPosition, uint32_t textLength,
                       const DWRITE_LINE_BREAKPOINT* lineBreakpoints);
    IFACEMETHODIMP SetBidiLevel(uint32_t textPosition, uint32_t textLength, uint8_t explicitLevel,
                                uint8_t resolvedLevel);
    IFACEMETHODIMP
    SetNumberSubstitution(uint32_t textPosition, uint32_t textLength,
                          IDWriteNumberSubstitution* numberSubstitution);

protected:
    Run* FetchNextRun(IN OUT uint32_t* textLength);
    void SetCurrentRun(uint32_t textPosition);
    void SplitCurrentRun(uint32_t splitPosition);

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

}
