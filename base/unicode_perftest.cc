#include "base/unicode.h"
#include "third_party/icu/icu_utf.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <print>
#include <string>
#include <string_view>

namespace {

// A mix of 1-, 2-, 3- and 4-byte sequences, repeated up to `bytes`.
std::string make_string(size_t bytes) {
    constexpr std::string_view kSample = "The quick brown fox jumps over the lazy dog. "
                                         "Ünïcödé façade naïve résumé. "
                                         "日本語のテキストと한국어 텍스트. "
                                         "😀🚀🎉 ";
    std::string s;
    s.reserve(bytes);
    while (s.size() + kSample.size() <= s.capacity()) {
        s += kSample;
    }
    return s;
}

std::string make_64_mb_string() { return make_string(64UZ << 20); }

std::string make_1024_mb_string() { return make_string(1024UZ << 20); }

void benchmark1(std::string_view s) {
    uint64_t checksum = 0;
    auto t1 = std::chrono::steady_clock::now();
    auto size = s.size();
    for (size_t i = 0; i < size;) {
        checksum += base::decode_valid_utf8(s, i);
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
    auto s16 = base::utf8_to_utf16(s);
    auto t2 = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::println("{} ms", duration);
    std::println("s16.size() = {}", s16.size());
}

void benchmark4(std::string_view s) {
    uint64_t checksum = 0;
    auto t1 = std::chrono::steady_clock::now();
    auto s32 = base::utf8_to_utf32(s);
    auto t2 = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::println("{} ms", duration);
    std::println("s32.size() = {}", s32.size());
}

void run_all(std::string_view s) {
    std::println("--- {} MB ---", (s.size() + (1UZ << 19)) >> 20);
    benchmark1(s);
    benchmark2(s);

    benchmark3(s);
    benchmark4(s);
}

}  // namespace

int main() {
    run_all(make_64_mb_string());
    run_all(make_1024_mb_string());
}
