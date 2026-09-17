#include "experiments/unicode/third_party/icu/icu_utf.h"
#include "experiments/unicode/unicode.h"
#include <chrono>
#include <cstdint>
#include <print>
#include <span>
#include <string>
#include <string_view>

namespace {

// A mix of 1-, 2-, 3- and 4-byte sequences, repeated to 64 MB.
std::string make_string() {
    constexpr std::string_view kSample = "The quick brown fox jumps over the lazy dog. "
                                         "Ünïcödé façade naïve résumé. "
                                         "日本語のテキストと한국어 텍스트. "
                                         "😀🚀🎉 ";
    std::string s;
    s.reserve(64 << 20);
    while (s.size() + kSample.size() <= s.capacity()) {
        s += kSample;
    }
    return s;
}

void benchmark1(std::string_view s) {
    uint64_t checksum = 0;
    auto t1 = std::chrono::steady_clock::now();
    auto size = s.size();
    for (size_t i = 0; i < size;) {
        checksum += unicode::decode_utf8(s, i);
    }
    auto t2 = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::println("{} ms", duration);
    std::println("checksum = {}", checksum);
}

void benchmark2(std::string_view s) {
    uint64_t checksum = 0;
    auto t1 = std::chrono::steady_clock::now();
    auto size = s.size();
    for (size_t i = 0; i < size;) {
        char32_t cp;
        U8_NEXT_UNSAFE(s, i, cp);
        // U8_INTERNAL_NEXT_OR_SUB(s, i, size, cp, 0xfffd);
        checksum += cp;
    }
    auto t2 = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::println("{} ms", duration);
    std::println("checksum = {}", checksum);
}

void benchmark3(std::string_view s) {
    uint64_t checksum = 0;
    auto t1 = std::chrono::steady_clock::now();
    auto s16 = unicode::utf8_to_utf16(s);
    auto t2 = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::println("{} ms", duration);
    std::println("s16.size() = {}", s16.size());
}

void benchmark4(std::string_view s) {
    uint64_t checksum = 0;
    auto t1 = std::chrono::steady_clock::now();
    auto s32 = unicode::utf8_to_utf32(s);
    auto t2 = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::println("{} ms", duration);
    std::println("s32.size() = {}", s32.size());
}

}  // namespace

int main() {
    auto s = make_string();
    benchmark1(s);
    benchmark2(s);

    benchmark3(s);
    benchmark4(s);
}
