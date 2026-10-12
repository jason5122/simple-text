#include "experiments/btree/btree.h"
#include <algorithm>
#include <fuzztest/fuzztest_core.h>
#include <gtest/gtest.h>
#include <utility>
#include <vector>

namespace btree {

namespace {
std::vector<int> sorted(std::vector<int> keys) {
    std::sort(keys.begin(), keys.end());
    return keys;
}
}  // namespace

// Inserting an arbitrary key sequence leaves a valid B-tree whose in-order traversal is the sorted
// input, duplicates included.
void InsertsKeepTreeValidAndSorted(const std::vector<int>& keys) {
    BTree tree;
    for (int k : keys) {
        tree.insert(k);
    }
    EXPECT_TRUE(tree.check_invariants());
    EXPECT_EQ(tree.keys(), sorted(keys));
}
FUZZ_TEST(BTreeFuzzTest, InsertsKeepTreeValidAndSorted);

// contains() is true for every inserted key and false for anything else.
void ContainsMatchesInsertedKeys(const std::vector<int>& keys, const std::vector<int>& probes) {
    BTree tree;
    for (int k : keys) {
        tree.insert(k);
    }
    for (int k : keys) {
        EXPECT_TRUE(tree.contains(k)) << k;
    }
    for (int p : probes) {
        bool inserted = std::find(keys.begin(), keys.end(), p) != keys.end();
        EXPECT_EQ(tree.contains(p), inserted) << p;
    }
}
FUZZ_TEST(BTreeFuzzTest, ContainsMatchesInsertedKeys);

// Moving a tree carries every key across intact, both by construction and by assignment over a
// tree that already holds keys of its own.
void MovePreservesKeys(const std::vector<int>& keys, const std::vector<int>& other_keys) {
    BTree source;
    for (int k : keys) {
        source.insert(k);
    }

    BTree constructed(std::move(source));
    EXPECT_TRUE(constructed.check_invariants());
    EXPECT_EQ(constructed.keys(), sorted(keys));

    BTree assigned;
    for (int k : other_keys) {
        assigned.insert(k);
    }
    assigned = std::move(constructed);
    EXPECT_TRUE(assigned.check_invariants());
    EXPECT_EQ(assigned.keys(), sorted(keys));
}
FUZZ_TEST(BTreeFuzzTest, MovePreservesKeys);

}  // namespace btree
