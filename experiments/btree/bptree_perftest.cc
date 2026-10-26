// Times each B+-tree operation over a million keys in random order. Build and run in release:
//
//   bin/ninja -C out/release bptree_perftest && out/release/bptree_perftest [seed]
//
// Each run picks a fresh seed for the key order and prints it; pass that seed back as the first
// argument to repeat the run exactly.

#include "experiments/btree/bptree.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <numeric>
#include <print>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr int kKeys = 1'000'000;
constexpr int kRangeWidth = 100;  // Keys per findRange query.

// Runs f once and prints its wall time, total and per operation.
template <typename F>
void measure(std::string_view name, int ops, F f) {
    auto start = std::chrono::steady_clock::now();
    f();
    std::chrono::duration<double, std::nano> elapsed = std::chrono::steady_clock::now() - start;
    std::println("{:<10}{:>10} ops{:>9.1f} ms{:>8.1f} ns/op", name, ops, elapsed.count() / 1e6,
                 elapsed.count() / ops);
}

}  // namespace

int main(int argc, char** argv) {
    std::uint32_t seed =
        argc > 1 ? static_cast<std::uint32_t>(std::stoul(argv[1])) : std::random_device{}();
    std::println("{} keys in random order, t = {}, seed {}", kKeys, bptree::t, seed);

    std::vector<int> keys(kKeys);
    std::iota(keys.begin(), keys.end(), 0);
    std::shuffle(keys.begin(), keys.end(), std::mt19937{seed});

    bptree::BPlusTree tree;
    int inserted = 0;
    int found = 0;
    int in_ranges = 0;
    int removed = 0;

    measure("insert", kKeys, [&] {
        for (int k : keys) {
            if (tree.insert(k)) inserted++;
        }
    });
    measure("find", kKeys, [&] {
        for (int k : keys) {
            if (tree.contains(k)) found++;
        }
    });
    measure("findRange", kKeys / kRangeWidth, [&] {
        for (int lb = 0; lb < kKeys; lb += kRangeWidth) {
            in_ranges += tree.find_range(lb, lb + kRangeWidth - 1).size();
        }
    });
    measure("remove", kKeys, [&] {
        for (int k : keys) {
            if (tree.remove(k)) removed++;
        }
    });

    // Every count should equal kKeys. Printing them also keeps the work observable, so the
    // optimizer cannot drop it.
    std::println("inserted {}, found {}, in ranges {}, removed {}", inserted, found, in_ranges,
                 removed);
}
