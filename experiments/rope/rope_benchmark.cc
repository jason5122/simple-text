#include "base/debug/profiler.h"
#include "base/rand_util.h"
#include "experiments/rope/rope.h"

#include <format>
#include <print>
#include <string>

namespace {

using rope::Rope;

constexpr size_t kAppendCount = 500'000;
constexpr size_t kRandomEditCount = 50'000;
constexpr size_t kReadRopeSize = 1'000'000;

// Types like a user, one character at a time, at the end of the document.
void benchmark_sequential_append() {
    auto prof = base::Profiler(std::format("Sequential append x{}", kAppendCount));
    Rope r;
    for (size_t i = 0; i < kAppendCount; ++i) {
        r.insert(r.size(), "x");
    }
}

// Pastes a small chunk at a random position, growing the rope from empty.
void benchmark_random_insert() {
    auto prof = base::Profiler(std::format("Random insert x{}", kRandomEditCount));
    Rope r;
    for (size_t i = 0; i < kRandomEditCount; ++i) {
        size_t pos = base::rand_int(0, static_cast<int>(r.size()));
        r.insert(pos, "12345678");
    }
}

// Deletes a small chunk at a random position out of a large, pre-built rope.
void benchmark_random_erase() {
    Rope r;
    for (size_t i = 0; i < kRandomEditCount; ++i) {
        r.insert(r.size(), "12345678");
    }

    auto prof = base::Profiler(std::format("Random erase x{}", kRandomEditCount));
    for (size_t i = 0; i < kRandomEditCount; ++i) {
        size_t pos = base::rand_int(0, static_cast<int>(r.size() - 8));
        r.erase(pos, 8);
    }
}

Rope build_large_rope(size_t target_size) {
    std::string text;
    text.reserve(target_size);
    size_t line_len = 0;
    for (size_t i = 0; i < target_size; ++i) {
        if (line_len >= 60) {
            text += '\n';
            line_len = 0;
        } else {
            text += static_cast<char>('a' + (i % 26));
            ++line_len;
        }
    }
    return Rope(text);
}

// The naive way to read out every character: one char_at() call per
// position, each a fresh descent from the root.
void benchmark_sequential_char_at_scan(const Rope& r) {
    auto prof = base::Profiler(std::format("Sequential char_at() scan x{}", r.size()));
    char sink = 0;
    for (size_t i = 0; i < r.size(); ++i) {
        sink ^= r.char_at(i);
    }
    std::println("  (checksum: {})", static_cast<int>(sink));
}

void benchmark_str_reconstruction(const Rope& r) {
    auto prof = base::Profiler(std::format("str() reconstruction x{}", r.size()));
    std::string s = r.str();
    std::println("  (reconstructed length: {})", s.size());
}

void benchmark_random_line_at(const Rope& r) {
    auto prof = base::Profiler(std::format("Random line_at() x{}", kRandomEditCount));
    for (size_t i = 0; i < kRandomEditCount; ++i) {
        size_t pos = base::rand_int(0, static_cast<int>(r.size()));
        static_cast<void>(r.line_at(pos));
    }
}

constexpr size_t kDenseNewlineRopeSize = 2'000'000;  // ~1,000,000 one-char lines.
constexpr size_t kHugeLineSize = 5'000'000;

// A file that's almost all short/blank lines -- e.g. a degenerate log
// format, or a file with one token per line.
Rope build_dense_newline_rope(size_t target_size) {
    std::string text;
    text.reserve(target_size);
    for (size_t i = 0; i + 1 < target_size; i += 2) {
        text += 'x';
        text += '\n';
    }
    return Rope(text);
}

void benchmark_random_line_at_dense_newlines(const Rope& r) {
    auto prof = base::Profiler(
        std::format("Random line_at() on {}-line rope x{}", r.line_count(), kRandomEditCount));
    for (size_t i = 0; i < kRandomEditCount; ++i) {
        size_t pos = base::rand_int(0, static_cast<int>(r.size()));
        static_cast<void>(r.line_at(pos));
    }
}

void benchmark_random_line_content_dense_newlines(const Rope& r) {
    auto prof = base::Profiler(std::format("Random line_content() on {}-line rope x{}",
                                           r.line_count(), kRandomEditCount));
    for (size_t i = 0; i < kRandomEditCount; ++i) {
        size_t line = base::rand_int(0, static_cast<int>(r.line_count() - 1));
        static_cast<void>(r.line_content(line));
    }
}

// The opposite pathology: a single, enormous line. line_content() has to
// walk it one char_at() call at a time (each its own O(log n) descent from
// the root), so extracting "the whole line" here does nearly as much work
// as extracting the whole document -- unlike str(), which is a flat
// leaf-chain walk. Compare this against benchmark_str_reconstruction on the
// same rope to see the gap.
Rope build_single_huge_line_rope(size_t target_size) {
    return Rope(std::string(target_size, 'x'));
}

void benchmark_line_content_huge_line(const Rope& r) {
    constexpr int kCalls = 10;
    auto prof =
        base::Profiler(std::format("line_content(0) on a {}-char line x{}", r.size(), kCalls));
    for (int i = 0; i < kCalls; ++i) {
        std::string line = r.line_content(0);
        static_cast<void>(line);
    }
}

}  // namespace

int main() {
    std::println("--- Mutation workloads ---");
    benchmark_sequential_append();
    benchmark_random_insert();
    benchmark_random_erase();

    std::println();
    std::println("--- Read workloads (on a {}-char rope) ---", kReadRopeSize);
    Rope big = build_large_rope(kReadRopeSize);
    benchmark_sequential_char_at_scan(big);
    benchmark_str_reconstruction(big);
    benchmark_random_line_at(big);

    std::println();
    std::println("--- Pathological newline workloads ---");
    Rope dense_newlines = build_dense_newline_rope(kDenseNewlineRopeSize);
    benchmark_random_line_at_dense_newlines(dense_newlines);
    benchmark_random_line_content_dense_newlines(dense_newlines);

    Rope huge_line = build_single_huge_line_rope(kHugeLineSize);
    benchmark_line_content_huge_line(huge_line);
    benchmark_str_reconstruction(huge_line);

    return 0;
}
