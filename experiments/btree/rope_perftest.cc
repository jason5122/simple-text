// Times the new rope (//experiments/btree) against the old one (//experiments/rope) on the same
// workload: a synthetic file of 100k lines, a million lookups, and 100k edits of each kind, plus
// a few megabyte-sized pastes and deletes. Build and run in release:
//
//   bin/ninja -C out/release rope_compare && out/release/rope_compare [seed]
//
// Each run picks a fresh seed for the text and the edit positions and prints it; pass that seed
// back as the first argument to repeat the run exactly.

#include "experiments/btree/rope.h"
#include "experiments/rope/rope.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <print>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr int kLines = 100'000;
constexpr int kLookups = 1'000'000;
constexpr int kEdits = 100'000;
constexpr int kPastes = 5;
constexpr size_t kPasteBytes = 1 << 20;

// Lines of varying length, so that chunk boundaries fall everywhere relative to lines.
std::string make_text(std::mt19937& rng) {
    std::string text;
    std::uniform_int_distribution<int> words(1, 12);
    for (int i = 0; i < kLines; i++) {
        text += std::format("line {}:", i);
        for (int w = words(rng); w > 0; w--) text += " lorem";
        text += '\n';
    }
    text.pop_back();
    return text;
}

std::vector<size_t> random_offsets(std::mt19937& rng, int count, size_t max) {
    std::uniform_int_distribution<size_t> dist(0, max);
    std::vector<size_t> out(count);
    for (size_t& v : out) v = dist(rng);
    return out;
}

// The same inputs for both ropes. The text is ASCII, so every offset is a boundary.
struct Workload {
    std::string text;
    std::vector<size_t> offsets;       // For offset_to_point.
    std::vector<size_t> rows;          // For point_to_offset and line.
    std::vector<size_t> edit_offsets;  // For inserts and erases.
    std::vector<size_t> paste_offsets;
    std::string paste;
};

// The two APIs differ in spelling; these adapters make the workload read the same for both.
size_t row_of(rope::Point p) { return static_cast<size_t>(p.row); }
size_t row_of(btree::Point p) { return p.line; }
size_t offset_of_row(const rope::Rope& r, size_t row) {
    return static_cast<size_t>(r.point_to_offset({static_cast<int>(row), 0}));
}
size_t offset_of_row(const btree::Rope& r, size_t row) { return r.point_to_offset({row, 0}); }
std::string line_of(const rope::Rope& r, size_t row) { return r.line(static_cast<int>(row)); }
std::string line_of(const btree::Rope& r, size_t row) { return r.line(row); }
std::string slice_of(const rope::Rope& r, size_t from, size_t to) {
    return r.slice(static_cast<int>(from), static_cast<int>(to));
}
std::string slice_of(const btree::Rope& r, size_t from, size_t to) { return r.substr(from, to); }
void insert_at(rope::Rope& r, size_t at, std::string_view text) {
    r.insert(static_cast<int>(at), text);
}
void insert_at(btree::Rope& r, size_t at, std::string_view text) { r.insert(at, text); }
void erase_range(rope::Rope& r, size_t from, size_t to) {
    r.erase(static_cast<int>(from), static_cast<int>(to));
}
void erase_range(btree::Rope& r, size_t from, size_t to) { r.erase(from, to); }
size_t longest_of(const rope::Rope& r) { return static_cast<size_t>(r.longest_line().bytes); }
size_t longest_of(const btree::Rope& r) { return r.longest_line(); }
size_t chunk_count(const rope::Rope& r) { return r.chunks().size(); }
size_t chunk_count(const btree::Rope& r) {
    size_t count = 0;
    for ([[maybe_unused]] const btree::Chunk& chunk : r.chunks()) count++;
    return count;
}

// One phase's result: its cost and a checksum of what it computed, so that the two ropes can be
// checked against each other as well as timed.
struct Phase {
    std::string_view name;
    int ops;
    double ns_per_op;
    unsigned long long checksum;
};

template <typename F>
Phase timed(std::string_view name, int ops, F f) {
    auto start = std::chrono::steady_clock::now();
    unsigned long long checksum = f();
    std::chrono::duration<double, std::nano> elapsed = std::chrono::steady_clock::now() - start;
    return {name, ops, elapsed.count() / ops, checksum};
}

template <typename Rope>
std::vector<Phase> run(const Workload& w) {
    std::vector<Phase> phases;
    Rope rope;
    phases.push_back(timed("build", kLines, [&] {
        rope = Rope(w.text);
        return static_cast<unsigned long long>(rope.size());
    }));
    phases.push_back(timed("offset_to_point", kLookups, [&] {
        unsigned long long rows = 0;
        for (size_t offset : w.offsets) rows += row_of(rope.offset_to_point(offset));
        return rows;
    }));
    phases.push_back(timed("point_to_offset", kLookups, [&] {
        unsigned long long offsets = 0;
        for (size_t row : w.rows) offsets += offset_of_row(rope, row);
        return offsets;
    }));
    phases.push_back(timed("line", kEdits, [&] {
        unsigned long long bytes = 0;
        for (int i = 0; i < kEdits; i++) bytes += line_of(rope, w.rows[i]).size();
        return bytes;
    }));
    // The old rope does not clamp ranges, so every range below is cut to the current size.
    auto size = [&] { return static_cast<size_t>(rope.size()); };
    phases.push_back(timed("substr 4K", kEdits, [&] {
        unsigned long long bytes = 0;
        for (int i = 0; i < kEdits; i++) {
            size_t from = std::min(w.edit_offsets[i], size());
            bytes += slice_of(rope, from, std::min(from + 4096, size())).size();
        }
        return bytes;
    }));
    phases.push_back(timed("insert random", kEdits, [&] {
        for (size_t offset : w.edit_offsets) insert_at(rope, offset, "word ");
        return static_cast<unsigned long long>(rope.size());
    }));
    size_t typing_start = w.edit_offsets[0];
    phases.push_back(timed("insert sequential", kEdits, [&] {
        for (int i = 0; i < kEdits; i++) {
            insert_at(rope, typing_start + i, i % 10 == 9 ? "\n" : "x");
        }
        return static_cast<unsigned long long>(rope.line_count());
    }));
    phases.push_back(timed("erase random", kEdits, [&] {
        for (size_t offset : w.edit_offsets) {
            size_t from = std::min(offset, size());
            erase_range(rope, from, std::min(from + 8, size()));
        }
        return static_cast<unsigned long long>(rope.size());
    }));
    phases.push_back(timed("paste 1M", kPastes, [&] {
        for (size_t offset : w.paste_offsets) insert_at(rope, std::min(offset, size()), w.paste);
        return static_cast<unsigned long long>(rope.size());
    }));
    phases.push_back(timed("delete 1M", kPastes, [&] {
        for (size_t offset : w.paste_offsets) {
            size_t from = std::min(offset, size());
            erase_range(rope, from, std::min(from + kPasteBytes, size()));
        }
        return static_cast<unsigned long long>(rope.size());
    }));
    phases.push_back(timed("longest_line", 1, [&] { return longest_of(rope); }));
    phases.push_back(timed("str", 1, [&] { return rope.str().size(); }));
    phases.push_back(timed("check_invariants", 1, [&] {
        return static_cast<unsigned long long>(rope.check_invariants());
    }));
    return phases;
}

}  // namespace

int main(int argc, char** argv) {
    std::uint32_t seed =
        argc > 1 ? static_cast<std::uint32_t>(std::stoul(argv[1])) : std::random_device{}();
    std::mt19937 rng{seed};
    Workload w;
    w.text = make_text(rng);
    w.offsets = random_offsets(rng, kLookups, w.text.size());
    w.rows = random_offsets(rng, kLookups, kLines - 1);
    w.edit_offsets = random_offsets(rng, kEdits, w.text.size());
    w.paste_offsets = random_offsets(rng, kPastes, w.text.size());
    w.paste = std::string(kPasteBytes, 'p');
    for (size_t i = 60; i < kPasteBytes; i += 61) w.paste[i] = '\n';
    std::println("{} bytes in {} lines, seed {}", w.text.size(), kLines, seed);
    std::println("");

    rope::Rope old_built(w.text);
    btree::Rope new_built(w.text);
    std::println("chunks after build: old {}, new {}", chunk_count(old_built),
                 chunk_count(new_built));
    std::println("");

    std::vector<Phase> old_rope = run<rope::Rope>(w);
    std::vector<Phase> new_rope = run<btree::Rope>(w);

    std::println("{:<18}{:>9}{:>14}{:>14}{:>8}", "phase", "ops", "old ns/op", "new ns/op",
                 "new/old");
    for (size_t i = 0; i < old_rope.size(); i++) {
        const Phase& a = old_rope[i];
        const Phase& b = new_rope[i];
        std::println(
            "{:<18}{:>9}{:>14.1f}{:>14.1f}{:>8}{}", a.name, a.ops, a.ns_per_op, b.ns_per_op,
            a.ns_per_op > 0 ? std::format("{:.2f}", b.ns_per_op / a.ns_per_op) : "-",
            a.checksum == b.checksum ? ""
                                     : std::format("  MISMATCH {} vs {}", a.checksum, b.checksum));
    }
}
