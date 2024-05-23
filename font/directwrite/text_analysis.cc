#include "text_analysis.h"
#include <cassert>

namespace font {
TextAnalysis::TextAnalysis(const wchar_t* text, uint32_t textLength, const wchar_t* localeName,
                           DWRITE_READING_DIRECTION readingDirection)
    : mTextLength(textLength), mText(text), mLocaleName(localeName),
      mReadingDirection(readingDirection), mCurrentRun(nullptr) {}

TextAnalysis::~TextAnalysis() {
    // delete runs, except mRunHead which is part of the TextAnalysis object
    for (Run* run = mRunHead.nextRun; run;) {
        Run* origRun = run;
        run = run->nextRun;
        delete origRun;
    }
}

STDMETHODIMP
TextAnalysis::GenerateResults(IDWriteTextAnalyzer* textAnalyzer, Run** runHead) {
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
TextAnalysis::GetTextAtPosition(uint32_t textPosition, OUT wchar_t const** textString,
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
TextAnalysis::GetTextBeforePosition(uint32_t textPosition, OUT wchar_t const** textString,
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
TextAnalysis::GetParagraphReadingDirection() {
    return mReadingDirection;
}

IFACEMETHODIMP TextAnalysis::GetLocaleName(uint32_t textPosition, uint32_t* textLength,
                                           wchar_t const** localeName) {
    return S_OK;
}

IFACEMETHODIMP
TextAnalysis::GetNumberSubstitution(uint32_t textPosition, OUT uint32_t* textLength,
                                    OUT IDWriteNumberSubstitution** numberSubstitution) {
    // We do not support number substitution.
    *numberSubstitution = nullptr;
    *textLength = mTextLength - textPosition;

    return S_OK;
}

// IDWriteTextAnalysisSink implementation

IFACEMETHODIMP
TextAnalysis::SetScriptAnalysis(uint32_t textPosition, uint32_t textLength,
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
TextAnalysis::SetLineBreakpoints(uint32_t textPosition, uint32_t textLength,
                                 const DWRITE_LINE_BREAKPOINT* lineBreakpoints) {
    return S_OK;
}

IFACEMETHODIMP TextAnalysis::SetBidiLevel(uint32_t textPosition, uint32_t textLength,
                                          uint8_t explicitLevel, uint8_t resolvedLevel) {
    return S_OK;
}

IFACEMETHODIMP
TextAnalysis::SetNumberSubstitution(uint32_t textPosition, uint32_t textLength,
                                    IDWriteNumberSubstitution* numberSubstitution) {
    return S_OK;
}

TextAnalysis::Run* TextAnalysis::FetchNextRun(IN OUT uint32_t* textLength) {
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

void TextAnalysis::SetCurrentRun(uint32_t textPosition) {
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

void TextAnalysis::SplitCurrentRun(uint32_t splitPosition) {
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
}
