#include "experiments/btree/bptree.h"

#include <algorithm>
#include <fuzztest/fuzztest_core.h>
#include <gtest/gtest.h>
#include <initializer_list>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace bptree {

namespace {

std::unique_ptr<Node> leaf(std::initializer_list<int> keys) {
    auto node = std::make_unique<Node>();
    for (int k : keys) node->key[node->n++] = k;
    return node;
}

template <typename... Children>
std::unique_ptr<Node> nonleaf(std::initializer_list<int> keys, Children... children) {
    static_assert(sizeof...(children) <= 2 * t);
    auto node = std::make_unique<Node>();
    node->leaf = false;
    for (int k : keys) node->key[node->n++] = k;
    int i = 0;
    ((node->c[i++] = std::move(children)), ...);
    return node;
}

void collect_leaves(Node* x, std::vector<Node*>& out) {
    if (x->leaf) {
        out.push_back(x);
        return;
    }
    for (int i = 0; i <= x->n; i++) collect_leaves(x->c[i].get(), out);
}

// Adopts a hand-built tree after linking its leaf chain (the book's Pn) left to right.
BPlusTree make_tree(std::unique_ptr<Node> root) {
    std::vector<Node*> leaves;
    collect_leaves(root.get(), leaves);
    for (size_t i = 0; i + 1 < leaves.size(); i++) leaves[i]->next = leaves[i + 1];
    return BPlusTree(std::move(root));
}

std::vector<int> keys_of(const Node* x) {
    std::vector<int> out;
    for (int i = 0; i < x->n; i++) out.push_back(x->key[i]);
    return out;
}

// Node-by-node structural comparison. The leaf chain is not compared; check_invariants covers
// it. `where` names the node under comparison, e.g. "root.c[1].c[0]".
testing::AssertionResult same_tree(const Node* actual,
                                   const Node* expected,
                                   const std::string& where = "root") {
    if (actual->leaf != expected->leaf || keys_of(actual) != keys_of(expected)) {
        return testing::AssertionFailure()
               << where << " is " << (actual->leaf ? "leaf " : "nonleaf ")
               << testing::PrintToString(keys_of(actual)) << ", expected "
               << (expected->leaf ? "leaf " : "nonleaf ")
               << testing::PrintToString(keys_of(expected));
    }
    if (actual->leaf) return testing::AssertionSuccess();
    for (int i = 0; i <= actual->n; i++) {
        testing::AssertionResult child = same_tree(actual->c[i].get(), expected->c[i].get(),
                                                   where + ".c[" + std::to_string(i) + "]");
        if (!child) return child;
    }
    return testing::AssertionSuccess();
}

std::vector<int> sorted_unique(std::vector<int> keys) {
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
    return keys;
}

// The instructor names of Figures 14.9 through 14.15 as ints in alphabetical order, spaced ten
// apart so that every gap between neighbours can be probed. Adams and Lamport are the two names
// the book inserts.
constexpr int Adams = 5;
constexpr int Brandt = 10;
constexpr int Califieri = 20;
constexpr int Crick = 30;
constexpr int Einstein = 40;
constexpr int ElSaid = 50;
constexpr int Gold = 60;
constexpr int Katz = 70;
constexpr int Kim = 80;
constexpr int Lamport = 85;
constexpr int Mozart = 90;
constexpr int Singh = 100;
constexpr int Srinivasan = 110;
constexpr int Wu = 120;

// Figure 14.9: the B+-tree for the instructor file, n = 4 in the book and t = 2 here.
BPlusTree figure_14_9() {
    return make_tree(
        nonleaf({Mozart},
                nonleaf({Einstein, Gold}, leaf({Brandt, Califieri, Crick}),
                        leaf({Einstein, ElSaid}), leaf({Gold, Katz, Kim})),
                nonleaf({Srinivasan}, leaf({Mozart, Singh}), leaf({Srinivasan, Wu}))));
}

// Figure 14.14: Figure 14.9 after inserting Adams, which split the first leaf.
BPlusTree figure_14_14() {
    return make_tree(nonleaf(
        {Mozart},
        nonleaf({Califieri, Einstein, Gold}, leaf({Adams, Brandt}), leaf({Califieri, Crick}),
                leaf({Einstein, ElSaid}), leaf({Gold, Katz, Kim})),
        nonleaf({Srinivasan}, leaf({Mozart, Singh}), leaf({Srinivasan, Wu}))));
}

// Figure 14.15: Figure 14.14 after inserting Lamport, which split a leaf and then its parent.
BPlusTree figure_14_15() {
    return make_tree(
        nonleaf({Gold, Mozart},
                nonleaf({Califieri, Einstein}, leaf({Adams, Brandt}), leaf({Califieri, Crick}),
                        leaf({Einstein, ElSaid})),
                nonleaf({Kim}, leaf({Gold, Katz}), leaf({Kim, Lamport})),
                nonleaf({Srinivasan}, leaf({Mozart, Singh}), leaf({Srinivasan, Wu}))));
}

// Figure 14.18: Figure 14.14 after deleting Srinivasan. The leaf coalesced with its left
// sibling, and the underfull parent then borrowed a pointer from its left sibling, moving Gold
// up to the root and Mozart down.
BPlusTree figure_14_18() {
    return make_tree(
        nonleaf({Gold},
                nonleaf({Califieri, Einstein}, leaf({Adams, Brandt}), leaf({Califieri, Crick}),
                        leaf({Einstein, ElSaid})),
                nonleaf({Mozart}, leaf({Gold, Katz, Kim}), leaf({Mozart, Singh, Wu}))));
}

// Figure 14.19: Figure 14.18 after deleting Singh and Wu. The leaf borrowed Kim from its left
// sibling, and the separator above them changed from Mozart to Kim.
BPlusTree figure_14_19() {
    return make_tree(nonleaf({Gold},
                             nonleaf({Califieri, Einstein}, leaf({Adams, Brandt}),
                                     leaf({Califieri, Crick}), leaf({Einstein, ElSaid})),
                             nonleaf({Kim}, leaf({Gold, Katz}), leaf({Kim, Mozart}))));
}

// Figure 14.20: Figure 14.19 after deleting Gold. The leaf coalesced with its right sibling, its
// parent then coalesced with the left nonleaf node, pulling Gold down from the root, and the
// root was left with one child and removed. Gold survives as a nonleaf key with no leaf entry.
BPlusTree figure_14_20() {
    return make_tree(nonleaf({Califieri, Einstein, Gold}, leaf({Adams, Brandt}),
                             leaf({Califieri, Crick}), leaf({Einstein, ElSaid}),
                             leaf({Katz, Kim, Mozart})));
}

}  // namespace

TEST(BPlusTreeTest, EmptyTreeContainsNothing) {
    BPlusTree tree;
    EXPECT_FALSE(tree.contains(0));
    EXPECT_FALSE(tree.contains(Mozart));
}

// Einstein, Gold, Mozart, and Srinivasan double as nonleaf separators, so they also check that a
// key equal to Ki is looked for under Pi+1 rather than Pi.
TEST(BPlusTreeTest, FindsEveryInstructorInFigure14_9) {
    BPlusTree tree = figure_14_9();
    for (int v : {Brandt, Califieri, Crick, Einstein, ElSaid, Gold, Katz, Kim, Mozart, Singh,
                  Srinivasan, Wu}) {
        EXPECT_TRUE(tree.contains(v)) << v;
    }
}

// One probe below the first key, one above the last, and one in each gap between neighbours.
TEST(BPlusTreeTest, DoesNotFindAbsentKeysInFigure14_9) {
    BPlusTree tree = figure_14_9();
    for (int v : {0, 15, 25, 35, 45, 55, 65, 75, 85, 95, 105, 115, 125}) {
        EXPECT_FALSE(tree.contains(v)) << v;
    }
}

TEST(BPlusTreeTest, FindRangeWalksAcrossLeaves) {
    BPlusTree tree = figure_14_9();
    EXPECT_EQ(tree.find_range(Crick, Katz),
              (std::vector<int>{Crick, Einstein, ElSaid, Gold, Katz}));
}

TEST(BPlusTreeTest, FindRangeBoundsNeedNotBeKeys) {
    BPlusTree tree = figure_14_9();
    EXPECT_EQ(tree.find_range(55, 85), (std::vector<int>{Gold, Katz, Kim}));
    EXPECT_EQ(tree.find_range(0, 200),
              (std::vector<int>{Brandt, Califieri, Crick, Einstein, ElSaid, Gold, Katz, Kim,
                                Mozart, Singh, Srinivasan, Wu}));
}

TEST(BPlusTreeTest, FindRangeOutsideTheKeysIsEmpty) {
    BPlusTree tree = figure_14_9();
    EXPECT_TRUE(tree.find_range(0, Adams).empty());     // Below Brandt.
    EXPECT_TRUE(tree.find_range(125, 200).empty());     // Above Wu: runs off the last leaf.
    EXPECT_TRUE(tree.find_range(Katz, Crick).empty());  // lb > ub.
    EXPECT_TRUE(BPlusTree().find_range(0, 200).empty());
}

TEST(BPlusTreeTest, InsertIntoEmptyTreeCreatesRootLeaf) {
    BPlusTree tree;
    EXPECT_TRUE(tree.insert(Mozart));
    EXPECT_TRUE(same_tree(tree.root(), leaf({Mozart}).get()));
    EXPECT_TRUE(tree.check_invariants());
}

TEST(BPlusTreeTest, InsertingAdamsSplitsLeafIntoFigure14_14) {
    BPlusTree tree = figure_14_9();
    EXPECT_TRUE(tree.insert(Adams));
    EXPECT_TRUE(same_tree(tree.root(), figure_14_14().root()));
    EXPECT_TRUE(tree.check_invariants());
}

TEST(BPlusTreeTest, InsertingLamportSplitsNonleafIntoFigure14_15) {
    BPlusTree tree = figure_14_14();
    EXPECT_TRUE(tree.insert(Lamport));
    EXPECT_TRUE(same_tree(tree.root(), figure_14_15().root()));
    EXPECT_TRUE(tree.check_invariants());
}

TEST(BPlusTreeTest, InsertRejectsDuplicateAndLeavesTreeUnchanged) {
    BPlusTree tree = figure_14_9();
    EXPECT_FALSE(tree.insert(Mozart));
    EXPECT_TRUE(same_tree(tree.root(), figure_14_9().root()));
}

TEST(BPlusTreeTest, CheckInvariantsRejectsUnlinkedLeafChain) {
    // A valid shape, but built without make_tree so no leaf points to the next one.
    BPlusTree tree(nonleaf({Mozart}, leaf({Brandt, Califieri}), leaf({Mozart, Singh})));
    EXPECT_FALSE(tree.check_invariants());
}

TEST(BPlusTreeTest, CheckInvariantsRejectsKeyUnderWrongSeparator) {
    // Mozart sits under P1, which may only hold keys below K1 = Mozart.
    BPlusTree tree = make_tree(nonleaf({Mozart}, leaf({Brandt, Mozart}), leaf({Singh, Wu})));
    EXPECT_FALSE(tree.check_invariants());
}

TEST(BPlusTreeTest, RemovingSrinivasanCoalescesAndRedistributesIntoFigure14_18) {
    BPlusTree tree = figure_14_14();
    EXPECT_TRUE(tree.remove(Srinivasan));
    EXPECT_TRUE(same_tree(tree.root(), figure_14_18().root()));
    EXPECT_TRUE(tree.check_invariants());
}

TEST(BPlusTreeTest, RemovingSinghAndWuBorrowsFromLeftLeafIntoFigure14_19) {
    BPlusTree tree = figure_14_18();
    EXPECT_TRUE(tree.remove(Singh));
    EXPECT_TRUE(tree.remove(Wu));
    EXPECT_TRUE(same_tree(tree.root(), figure_14_19().root()));
    EXPECT_TRUE(tree.check_invariants());
}

TEST(BPlusTreeTest, RemovingGoldCoalescesUpToTheRootIntoFigure14_20) {
    BPlusTree tree = figure_14_19();
    EXPECT_TRUE(tree.remove(Gold));
    EXPECT_TRUE(same_tree(tree.root(), figure_14_20().root()));
    EXPECT_TRUE(tree.check_invariants());
    EXPECT_FALSE(tree.contains(Gold));
}

// The book spells out borrowing from the previous sibling and leaves the next-sibling case as
// "symmetric". Here the first leaf, which has no previous sibling, borrows Mozart from the next
// one and the separator above them becomes Singh.
TEST(BPlusTreeTest, RemoveBorrowsFromNextLeafWhenThereIsNoPreviousOne) {
    BPlusTree tree =
        make_tree(nonleaf({Mozart}, leaf({Brandt, Califieri}), leaf({Mozart, Singh, Srinivasan})));
    EXPECT_TRUE(tree.remove(Brandt));
    EXPECT_TRUE(same_tree(
        tree.root(),
        make_tree(nonleaf({Singh}, leaf({Califieri, Mozart}), leaf({Singh, Srinivasan}))).root()));
    EXPECT_TRUE(tree.check_invariants());
}

// The nonleaf half of the symmetric case, with plain numbers since the instructor names run out.
// Removing 10 coalesces the first two leaves, which leaves their parent with one pointer. Its
// only sibling is full, so the parent borrows that sibling's first pointer; the separator 50
// moves down into the parent and 60 moves up to the root.
TEST(BPlusTreeTest, RemoveBorrowsFromNextNonleafWhenThereIsNoPreviousOne) {
    BPlusTree tree = make_tree(nonleaf(
        {50}, nonleaf({20}, leaf({10, 15}), leaf({20, 25})),
        nonleaf({60, 70, 80}, leaf({50, 55}), leaf({60, 65}), leaf({70, 75}), leaf({80, 85}))));
    EXPECT_TRUE(tree.remove(10));
    EXPECT_TRUE(same_tree(
        tree.root(),
        make_tree(nonleaf({60}, nonleaf({50}, leaf({15, 20, 25}), leaf({50, 55})),
                          nonleaf({70, 80}, leaf({60, 65}), leaf({70, 75}), leaf({80, 85}))))
            .root()));
    EXPECT_TRUE(tree.check_invariants());
}

TEST(BPlusTreeTest, RemovingEveryKeyEmptiesTheTree) {
    BPlusTree tree;
    for (int k : {1, 2, 3, 4, 5}) tree.insert(k);
    for (int k : {1, 2, 3, 4, 5}) EXPECT_TRUE(tree.remove(k)) << k;
    EXPECT_EQ(tree.root(), nullptr);
    EXPECT_TRUE(tree.keys().empty());
    EXPECT_TRUE(tree.insert(3));
    EXPECT_TRUE(tree.contains(3));
}

TEST(BPlusTreeTest, RemoveReturnsFalseForAbsentKeyAndLeavesTreeUnchanged) {
    BPlusTree tree = figure_14_9();
    EXPECT_FALSE(tree.remove(Adams));
    EXPECT_TRUE(same_tree(tree.root(), figure_14_9().root()));
}

// Inserting an arbitrary key sequence leaves a valid B+-tree whose leaf chain reads as the sorted
// input with duplicates dropped, and insert reports whether each key was new.
void InsertsKeepTreeValidAndSorted(const std::vector<int>& keys) {
    BPlusTree tree;
    std::set<int> seen;
    for (int k : keys) {
        EXPECT_EQ(tree.insert(k), seen.insert(k).second) << k;
    }
    EXPECT_TRUE(tree.check_invariants());
    EXPECT_EQ(tree.keys(), sorted_unique(keys));
}
FUZZ_TEST(BPlusTreeFuzzTest, InsertsKeepTreeValidAndSorted);

// contains() is true for every inserted key and false for anything else.
void ContainsMatchesInsertedKeys(const std::vector<int>& keys, const std::vector<int>& probes) {
    BPlusTree tree;
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
FUZZ_TEST(BPlusTreeFuzzTest, ContainsMatchesInsertedKeys);

// find_range returns exactly the inserted keys within [lb, ub], ascending. Keys and bounds are
// drawn from a small range so that ranges usually cover some keys and bounds often hit one.
void FindRangeMatchesSortedSlice(const std::vector<int>& keys, int lb, int ub) {
    BPlusTree tree;
    for (int k : keys) {
        tree.insert(k);
    }
    std::vector<int> expected;
    for (int k : sorted_unique(keys)) {
        if (lb <= k && k <= ub) expected.push_back(k);
    }
    EXPECT_EQ(tree.find_range(lb, ub), expected);
}
FUZZ_TEST(BPlusTreeFuzzTest, FindRangeMatchesSortedSlice)
    .WithDomains(fuzztest::VectorOf(fuzztest::InRange(0, 100)),
                 fuzztest::InRange(-5, 105),
                 fuzztest::InRange(-5, 105));

// Random interleaved inserts and removes agree with a std::set at every step, and the tree stays
// valid throughout. Keys come from a small range so that removes usually hit and nodes split and
// coalesce often.
void InterleavedInsertsAndRemovesMatchSetOracle(const std::vector<std::pair<bool, int>>& ops) {
    BPlusTree tree;
    std::set<int> oracle;
    for (auto [is_insert, k] : ops) {
        if (is_insert) {
            EXPECT_EQ(tree.insert(k), oracle.insert(k).second) << "insert " << k;
        } else {
            EXPECT_EQ(tree.remove(k), oracle.erase(k) == 1) << "remove " << k;
        }
        ASSERT_TRUE(tree.check_invariants()) << (is_insert ? "insert " : "remove ") << k;
    }
    EXPECT_EQ(tree.keys(), std::vector<int>(oracle.begin(), oracle.end()));
}
FUZZ_TEST(BPlusTreeFuzzTest, InterleavedInsertsAndRemovesMatchSetOracle)
    .WithDomains(fuzztest::VectorOf(fuzztest::PairOf(fuzztest::Arbitrary<bool>(),
                                                     fuzztest::InRange(0, 50))));

}  // namespace bptree
