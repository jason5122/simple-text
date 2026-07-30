#include "base/check.h"
#include "base/win/scoped_com_initializer.h"

namespace base::win {

ScopedCOMInitializer::ScopedCOMInitializer() { initialize(COINIT_APARTMENTTHREADED); }

ScopedCOMInitializer::ScopedCOMInitializer(SelectMTA) { initialize(COINIT_MULTITHREADED); }

ScopedCOMInitializer::~ScopedCOMInitializer() {
    if (succeeded()) {
        ::CoUninitialize();
    }
}

void ScopedCOMInitializer::initialize(COINIT init) {
    // COINIT_DISABLE_OLE1DDE is always added per Microsoft guidance:
    // https://learn.microsoft.com/windows/win32/learnwin32/initializing-the-com-library
    hr_ = ::CoInitializeEx(nullptr, init | COINIT_DISABLE_OLE1DDE);
    // Changing the apartment model on an already-initialized thread is a caller bug.
    DCHECK_NE(RPC_E_CHANGED_MODE, hr_);
}

}  // namespace base::win
