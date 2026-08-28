#include "base/debug/profiler.h"
#include "base/rand_util.h"
#include "experiments/rope/rope.h"

#include <cmath>
#include <format>
#include <print>
#include <string>
#include <string_view>

namespace {

using rope::LineWidthMetric;
using rope::Rope;

constexpr size_t kAppendCount = 500'000;
constexpr size_t kRandomEditCount = 50'000;
constexpr size_t kReadRopeSize = 1'000'000;

// Stand-in for a real font backend, playing the same role
// rope_unittest.cc's reference_char_width() does -- benchmarks only need
// this to do real per-character work, not to be visually meaningful.
double fake_char_width(char c) { return c == '\t' ? 4.0 : 1.0; }

LineWidthMetric measure_leaf_width(std::string_view text) {
    LineWidthMetric result;
    double current = 0;
    for (char c : text) {
        if (c == '\n') {
            result.widest_complete_line = std::max(result.widest_complete_line, current);
            if (result.newline_count == 0) {
                result.leading_open = current;
            }
            ++result.newline_count;
            current = 0;
        } else {
            current += fake_char_width(c);
        }
    }
    result.trailing_open = current;
    if (result.newline_count == 0) {
        result.leading_open = current;
    }
    return result;
}

// What a real layout pass does after a batch of edits: measure whatever's
// currently dirty and push it back in. Deliberately not timed on its own
// below -- it's a building block for the benchmarks that are.
void remeasure_all(Rope& r) {
    for (Rope::LeafHandle leaf : r.dirty_leaves()) {
        r.set_leaf_width(leaf, measure_leaf_width(r.leaf_text(leaf)));
    }
}

// Mirrors rope.cc's rows_for() (see SoftWrapMetric's comment in rope.h for
// why row counts are derived from logical-line widths, not tracked
// directly).
size_t rows_for(double width, double wrap_width) {
    return width > 0 ? static_cast<size_t>(std::ceil(width / wrap_width)) : size_t{1};
}

rope::SoftWrapMetric measure_leaf_wrap(std::string_view text, double wrap_width) {
    rope::SoftWrapMetric result;
    double current = 0;
    for (char c : text) {
        if (c == '\n') {
            if (result.newline_count == 0) {
                result.leading_open_width = current;
            } else {
                result.complete_rows += rows_for(current, wrap_width);
            }
            ++result.newline_count;
            current = 0;
        } else {
            current += fake_char_width(c);
        }
    }
    result.trailing_open_width = current;
    if (result.newline_count == 0) {
        result.leading_open_width = current;
    }
    return result;
}

void remeasure_all_wrap(Rope& r, Rope::WrapHandle handle, double wrap_width) {
    for (Rope::LeafHandle leaf : r.dirty_leaves_for_wrap(handle)) {
        r.set_leaf_wrap(leaf, handle, measure_leaf_wrap(r.leaf_text(leaf), wrap_width));
    }
}

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

// Baseline for the incremental benchmark below: measuring every leaf from
// scratch, the cost an editor would pay on every edit without dirty
// tracking (or once, e.g. right after loading a file nothing has been
// laid out yet). `r` must be freshly built -- every leaf starts dirty by
// construction, so this is really "cost of measuring the whole document."
void benchmark_full_remeasure(Rope& r) {
    auto prof = base::Profiler(std::format("Full remeasure (all leaves dirty) on {}-char rope",
                                           r.size()));
    remeasure_all(r);
}

// The O(total leaves) pointer-chase dirty_leaves() does to find what's
// dirty (see its comment in rope.h -- deliberately not optimized to skip
// clean subtrees yet). Run on an already-fully-measured rope, so this
// isolates the scan cost from any actual measurement work.
void benchmark_dirty_leaves_scan_when_clean(Rope& r) {
    remeasure_all(r);
    auto prof = base::Profiler(
        std::format("dirty_leaves() scan (nothing dirty) on {}-char rope x{}", r.size(), 1'000));
    for (int i = 0; i < 1'000; ++i) {
        auto leaves = r.dirty_leaves();
        static_cast<void>(leaves);
    }
}

// Isolates insert()'s own cost on this same tightly-packed shape, with no
// remeasuring at all -- build_large_rope() builds its whole string via one
// insert() call, so every leaf lands exactly at the kMaxLeafLen cap (rope.h
// documents it) instead of the mixed fill levels organic typing leaves
// behind. A scattered single-char insert into that shape overflows some
// leaf on nearly every call, so this isn't the same baseline as
// benchmark_random_insert() above (which grows its rope from empty, so
// most of its inserts land while the tree is still small) -- it's what
// lets benchmark_incremental_remeasure_after_edits() below attribute its
// cost correctly instead of blaming dirty tracking for splitting cost that
// insert() would pay regardless.
void benchmark_random_insert_into_packed_rope(Rope& r) {
    auto prof = base::Profiler(
        std::format("Random insert (no remeasure) on {}-char rope x{}", r.size(), kRandomEditCount));
    for (size_t i = 0; i < kRandomEditCount; ++i) {
        size_t pos = base::rand_int(0, static_cast<int>(r.size()));
        r.insert(pos, "x");
    }
}

// The actual payoff of leaf handles + dirty tracking: after a single small
// edit, how much work does bringing widths back up to date take, and does
// that cost grow with document size? Every edit marks only the 1-2 leaves
// it touched dirty (each up to kMaxLeafLen characters -- rope.h documents
// the cap), and set_leaf_width()'s up-walk is O(depth). Both are bounded by
// constants that don't depend on total document size, so per-edit cost
// should come out roughly the same on a huge document as on a small one --
// contrast with benchmark_full_remeasure() above, which is deliberately
// O(document size). Call this on ropes of very different sizes (see main())
// and compare the per-edit average to see the flatness directly, rather
// than just asserting it.
void benchmark_incremental_remeasure_after_edits(Rope& r) {
    remeasure_all(r);  // Start clean, like a real editor between keystrokes.

    auto prof = base::Profiler(std::format(
        "insert() + incremental remeasure on {}-char rope x{}", r.size(), kRandomEditCount));
    for (size_t i = 0; i < kRandomEditCount; ++i) {
        size_t pos = base::rand_int(0, static_cast<int>(r.size()));
        r.insert(pos, "x");
        remeasure_all(r);
    }
}

// Cost of adding a second (or Nth) simultaneous wrap width to an
// already-large document -- e.g. opening a second split pane on a huge
// file. O(total nodes): every node in the tree gets a new slot (see
// register_wrap_width()'s comment in rope.h and
// reverse_engineering/soft_wrap_caching.md for why this mirrors Sublime
// Text's TokenStorage::addLayout).
void benchmark_register_wrap_width(Rope& r) {
    auto prof =
        base::Profiler(std::format("register_wrap_width() on {}-char rope", r.size()));
    Rope::WrapHandle handle = r.register_wrap_width(80);
    static_cast<void>(handle);
}

// The mirror image: closing a view. O(total nodes), same shape as
// register above but shifting every node's slot array down to stay
// compact (mirrors TokenStorage::removeLayout).
void benchmark_unregister_wrap_width(Rope& r) {
    Rope::WrapHandle handle = r.register_wrap_width(80);
    auto prof =
        base::Profiler(std::format("unregister_wrap_width() on {}-char rope", r.size()));
    r.unregister_wrap_width(handle);
}

// Baseline for the incremental wrap benchmark below, same role as
// benchmark_full_remeasure() plays for widths: measuring every leaf's
// wrap layout from scratch. `r` must be freshly built.
void benchmark_full_remeasure_wrap(Rope& r) {
    Rope::WrapHandle handle = r.register_wrap_width(80);
    auto prof = base::Profiler(
        std::format("Full wrap remeasure (all leaves dirty) on {}-char rope", r.size()));
    remeasure_all_wrap(r, handle, 80);
}

// The wrap-specific counterpart to benchmark_incremental_remeasure_after_
// edits(): does bringing one wrap width's layout back up to date after a
// single edit stay cheap and roughly flat regardless of document size?
// Same reasoning applies -- set_leaf_wrap()'s up-walk is O(depth),
// independent of document size.
void benchmark_incremental_remeasure_wrap_after_edits(Rope& r) {
    Rope::WrapHandle handle = r.register_wrap_width(80);
    remeasure_all_wrap(r, handle, 80);

    auto prof = base::Profiler(std::format(
        "insert() + incremental wrap remeasure on {}-char rope x{}", r.size(), kRandomEditCount));
    for (size_t i = 0; i < kRandomEditCount; ++i) {
        size_t pos = base::rand_int(0, static_cast<int>(r.size()));
        r.insert(pos, "x");
        remeasure_all_wrap(r, handle, 80);
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

    std::println();
    std::println("--- Layout workloads (leaf handles + dirty tracking, on a {}-char rope) ---",
                 kReadRopeSize);
    Rope layout_full = build_large_rope(kReadRopeSize);
    benchmark_full_remeasure(layout_full);

    Rope layout_clean = build_large_rope(kReadRopeSize);
    benchmark_dirty_leaves_scan_when_clean(layout_clean);

    Rope layout_insert_only = build_large_rope(kReadRopeSize);
    benchmark_random_insert_into_packed_rope(layout_insert_only);

    Rope layout_incremental = build_large_rope(kReadRopeSize);
    benchmark_incremental_remeasure_after_edits(layout_incremental);

    // Same workload on a much smaller document -- if incremental remeasure
    // is genuinely independent of document size (not just of leaf count
    // within one already-large rope), the per-edit average here should
    // land close to the run above, not shrink proportionally with the
    // ~50x-smaller document.
    constexpr size_t kSmallLayoutRopeSize = 20'000;
    std::println();
    Rope layout_incremental_small = build_large_rope(kSmallLayoutRopeSize);
    benchmark_incremental_remeasure_after_edits(layout_incremental_small);

    std::println();
    std::println("--- Soft wrap workloads (WrapHandle registry, on a {}-char rope) ---",
                 kReadRopeSize);
    Rope wrap_register = build_large_rope(kReadRopeSize);
    benchmark_register_wrap_width(wrap_register);

    Rope wrap_unregister = build_large_rope(kReadRopeSize);
    benchmark_unregister_wrap_width(wrap_unregister);

    Rope wrap_full = build_large_rope(kReadRopeSize);
    benchmark_full_remeasure_wrap(wrap_full);

    Rope wrap_incremental = build_large_rope(kReadRopeSize);
    benchmark_incremental_remeasure_wrap_after_edits(wrap_incremental);

    return 0;
}
