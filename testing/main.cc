#include <cstdlib>

#include <fuzztest/init_fuzztest.h>
#include <gtest/gtest.h>

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    fuzztest::ParseAbslFlags(argc, argv);
    // Registering FUZZ_TESTs as gtest cases costs ~1s each in unit-test mode,
    // so keep them out of the default run and opt in with RUN_FUZZTESTS. This
    // also gates --fuzz / --list_fuzz_tests, which need the fuzztest registry.
    if (std::getenv("RUN_FUZZTESTS")) {
        fuzztest::InitFuzzTest(&argc, &argv);
    }
    return RUN_ALL_TESTS();
}
