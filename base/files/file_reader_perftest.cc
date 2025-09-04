#include <gtest/gtest.h>

#include "base/debug/profiler.h"
#include "base/files/file_reader.h"

namespace base {

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

}  // namespace

// See discussion below on `fwrite(_, 1, N, _)` vs. `fwrite(_, N, 1, _)`.
// https://stackoverflow.com/a/21769967/14698275
TEST(FileReaderPerfTest, ReadFile1) {
    auto p1 = Profiler{"Write 1GB file"};
    WriteFile(kFileName, kStr1Gb);
    p1.stop_mili();

    auto p2 = Profiler{"Read 1GB file"};
    std::string contents = ReadFile(kFileName);
    p2.stop_mili();

    std::remove(kFileName.data());
}

}  // namespace base
