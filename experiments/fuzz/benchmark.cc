#include "base/debug/profiler.h"
#include "base/rand_util.h"
#include <set>
#include <spdlog/spdlog.h>
#include <string_view>

#include "clrs.h"
#include "custom.h"
// #include "red_black_tree.h"

namespace {

void benchmark_std_set(const std::vector<int>& nums) {
    auto prof = base::Profiler("std::set");
    std::set<int> set;
    for (int k : nums) set.insert(k);
}

// void benchmark_red_black_tree(const std::vector<int>& nums) {
//     auto prof = base::Profiler("editor::RedBlackTree");
//     editor::NodeArena arena;
//     editor::RedBlackTree t;
//     for (int k : nums) {
//         size_t at = static_cast<size_t>(k) % (t.length() + 1);
//         t = t.insert(arena, k, {.length = 1});
//     }
// }

void benchmark_red_black_tree(const std::vector<int>& nums) {
    auto prof = base::Profiler("editor::RedBlackTree");
    editor::RedBlackTree t;
    for (int k : nums) {
        size_t at = static_cast<size_t>(k) % (t.length() + 1);
        t.insert_at(k, {.length = 1});
    }
}

void benchmark_third_party_red_black_tree(const std::vector<int>& nums) {
    auto prof = base::Profiler("CLRS");
    RedBlackTree t;
    for (int k : nums) t.insert(k);
}

}  // namespace

int main() {
    for (int N = 1'000; N <= 1'000'000; N *= 10) {
        std::cout << "N = " << N << '\n';

        std::vector<int> nums(N);
        for (size_t i = 0; i < N; i++) nums[i] = base::rand_int(0, 4096);

        benchmark_std_set(nums);
        benchmark_third_party_red_black_tree(nums);
        benchmark_red_black_tree(nums);

        std::cout << '\n';
    }
}
