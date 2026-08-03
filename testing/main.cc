#include <fuzztest/init_fuzztest.h>
#include <gtest/gtest.h>

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    fuzztest::ParseAbslFlags(argc, argv);
    fuzztest::InitFuzzTest(&argc, &argv);
    return RUN_ALL_TESTS();
}
