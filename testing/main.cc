#include "build/build_config.h"
#include <gtest/gtest.h>

#if BUILDFLAG(IS_WIN)
#include <ole2.h>
#endif

int main(int argc, char** argv) {
#if BUILDFLAG(IS_WIN)
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
#endif

    testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

#if BUILDFLAG(IS_WIN)
    CoUninitialize();
#endif

    return result;
}
