#pragma once

#include <objbase.h>

namespace base::win {

class ScopedCOMInitializer {
public:
    // Tag to select MTA initialization instead of the default STA.
    enum SelectMTA { kMTA };

    ScopedCOMInitializer();                    // STA
    explicit ScopedCOMInitializer(SelectMTA);  // MTA
    ~ScopedCOMInitializer();

    ScopedCOMInitializer(const ScopedCOMInitializer&) = delete;
    ScopedCOMInitializer& operator=(const ScopedCOMInitializer&) = delete;

    bool succeeded() const { return SUCCEEDED(hr_); }
    HRESULT hr() const { return hr_; }

private:
    void initialize(COINIT init);

    HRESULT hr_ = S_OK;
};

}  // namespace base::win
