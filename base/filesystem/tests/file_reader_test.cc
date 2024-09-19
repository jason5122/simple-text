#include "base/filesystem/file_reader.h"
#include <gtest/gtest.h>

// TODO: Debug use; remove this.
#include "util/profile_util.h"

namespace {

constexpr auto operator*(const std::string_view& sv, size_t times) {
    std::string result;
    for (size_t i = 0; i < times; ++i) {
        result += sv;
    }
    return result;
}

constexpr std::string_view kLongLine =
    R"(aldksfjasldkfjalksdfjlkadsfjklasfjlskfoiewfnfmxcnvadslfkjasnkli02ijdsfklasjdflafoiwenlskdafnlksdfln
)";

constexpr std::string_view kFileName = "1gb.txt";
const std::string kStr1Gb = kLongLine * 10000000;

}

// See discussion below on `fwrite(_, 1, N, _)` vs. `fwrite(_, N, 1, _)`.
// https://stackoverflow.com/a/21769967/14698275
TEST(FileReaderTest, ReadFile1) {
    {
        PROFILE_BLOCK_WITH_DURATION("Write 1GB file", std::chrono::milliseconds);
        base::WriteFile(kFileName, kStr1Gb);
    }

    {
        PROFILE_BLOCK_WITH_DURATION("Read 1GB file", std::chrono::milliseconds);
        std::string contents = base::ReadFile(kFileName);
    }

    remove(kFileName.data());
}
