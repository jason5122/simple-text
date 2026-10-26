#include "experiments/btree/sum_tree.h"
#include "experiments/btree/sum_tree_svg.h"

#include <format>
#include <fuzztest/fuzztest_core.h>
#include <gtest/gtest.h>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace btree {

namespace {

// The (count, sum) summary of sum_tree_count_and_sum_example.png. Along count a seek is CLRS's
// OS-SELECT; along sum it is a select weighted by the items' values.
struct CountSum {
    int count = 0;
    int sum = 0;
    bool operator==(const CountSum&) const = default;
    friend CountSum operator+(const CountSum& a, const CountSum& b) {
        return {a.count + b.count, a.sum + b.sum};
    }
};

// An item that measures itself: a number counts once and adds its value to the sum.
struct Number {
    int value = 0;
    CountSum summary() const { return {1, value}; }
    bool operator==(const Number&) const = default;
};

// t = 2 keeps the hand-built trees small, with two or three entries per node like the diagram.
constexpr int min_degree = 2;
using Tree = SumTree<Number, min_degree>;
using Node = Tree::Node;

std::unique_ptr<Node> leaf(std::initializer_list<int> items) {
    auto node = std::make_unique<Node>();
    for (int v : items) {
        node->item[node->n] = Number{v};
        node->summary[node->n] = Number{v}.summary();
        node->n++;
    }
    return node;
}

CountSum total_of(const Node& x) {
    CountSum s;
    for (int i = 0; i < x.n; i++) s = s + x.summary[i];
    return s;
}

template <typename... Children>
std::unique_ptr<Node> nonleaf(Children... children) {
    static_assert(sizeof...(children) <= 2 * min_degree - 1);
    auto node = std::make_unique<Node>();
    node->leaf = false;
    ((node->summary[node->n] = total_of(*children), node->c[node->n] = std::move(children),
      node->n++),
     ...);
    return node;
}

std::vector<int> items_of(const Node& x) {
    std::vector<int> out;
    for (int i = 0; i < x.n; i++) out.push_back(x.item[i].value);
    return out;
}

std::vector<int> values_of(const std::vector<Number>& items) {
    std::vector<int> out;
    for (const Number& item : items) out.push_back(item.value);
    return out;
}

std::vector<std::pair<int, int>> summaries_of(const Node& x) {
    std::vector<std::pair<int, int>> out;
    for (int i = 0; i < x.n; i++) out.push_back({x.summary[i].count, x.summary[i].sum});
    return out;
}

// Node-by-node structural comparison: a leaf must hold the same items, a nonleaf node the same
// entry summaries with the same children beneath. Leaf summaries are not compared;
// check_invariants covers them. `where` names the node under comparison, e.g. "root.c[1].c[0]".
testing::AssertionResult same_tree(const Node* actual,
                                   const Node* expected,
                                   const std::string& where = "root") {
    if (actual->leaf != expected->leaf) {
        return testing::AssertionFailure()
               << where << " is " << (actual->leaf ? "a leaf" : "nonleaf") << ", expected "
               << (expected->leaf ? "a leaf" : "nonleaf");
    }
    if (actual->leaf) {
        if (items_of(*actual) != items_of(*expected)) {
            return testing::AssertionFailure()
                   << where << " holds " << testing::PrintToString(items_of(*actual))
                   << ", expected " << testing::PrintToString(items_of(*expected));
        }
        return testing::AssertionSuccess();
    }
    if (summaries_of(*actual) != summaries_of(*expected)) {
        return testing::AssertionFailure()
               << where << " summarizes " << testing::PrintToString(summaries_of(*actual))
               << ", expected " << testing::PrintToString(summaries_of(*expected));
    }
    for (int i = 0; i < actual->n; i++) {
        testing::AssertionResult child = same_tree(actual->c[i].get(), expected->c[i].get(),
                                                   where + ".c[" + std::to_string(i) + "]");
        if (!child) return child;
    }
    return testing::AssertionSuccess();
}

// The k-th item, counting from 1: the first whose end has count >= k.
template <typename AnyTree>
AnyTree::iterator kth(const AnyTree& tree, int k) {
    return tree.seek([k](const CountSum& s) { return s.count >= k; });
}

// The first item at which the running sum reaches `target`.
Tree::iterator sum_reaches(const Tree& tree, int target) {
    return tree.seek([target](const CountSum& s) { return s.sum >= target; });
}

// The tree of sum_tree_count_and_sum_example.png: the digits of pi as items A through H, with a
// (count, sum) at every nonleaf entry and (8, 31) over the whole tree.
Tree diagram() {
    return Tree(nonleaf(nonleaf(leaf({3, 1}), leaf({4, 1})), nonleaf(leaf({5, 9}), leaf({2, 6}))));
}

int count_of(std::string_view text, std::string_view needle) {
    int count = 0;
    for (size_t at = text.find(needle); at != std::string_view::npos;
         at = text.find(needle, at + needle.size())) {
        count++;
    }
    return count;
}

}  // namespace

}  // namespace btree

// std::formatter specializations must sit outside any other namespace.
template <>
struct std::formatter<btree::CountSum> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const btree::CountSum& s, auto& ctx) const {
        return std::format_to(ctx.out(), "({}, {})", s.count, s.sum);
    }
};

template <>
struct std::formatter<btree::Number> : std::formatter<int> {
    auto format(const btree::Number& n, auto& ctx) const {
        return std::formatter<int>::format(n.value, ctx);
    }
};

namespace btree {

TEST(SumTreeTest, EmptyTreeHasTheIdentitySummaryAndNoItems) {
    Tree tree;
    EXPECT_EQ(tree.summary(), CountSum{});
    EXPECT_TRUE(values_of(tree.items()).empty());
    EXPECT_TRUE(tree.begin() == tree.end());
    EXPECT_TRUE(kth(tree, 1) == tree.end());
    EXPECT_TRUE(tree.check_invariants());
}

TEST(SumTreeTest, DiagramSummariesAddUp) {
    Tree tree = diagram();
    EXPECT_EQ(tree.summary(), (CountSum{8, 31}));
    EXPECT_EQ(values_of(tree.items()), (std::vector<int>{3, 1, 4, 1, 5, 9, 2, 6}));
    EXPECT_TRUE(tree.check_invariants());
}

TEST(SumTreeTest, SeekByCountSelectsTheKthItem) {
    Tree tree = diagram();
    std::vector<int> items = {3, 1, 4, 1, 5, 9, 2, 6};
    for (int k = 1; k <= 8; k++) {
        Tree::iterator c = kth(tree, k);
        ASSERT_TRUE(c != tree.end()) << k;
        EXPECT_EQ(c->value, items[k - 1]) << k;
        EXPECT_EQ(c.start().count, k - 1) << k;
    }
}

TEST(SumTreeTest, SeekPastTheLastItemIsAtTheEnd) {
    Tree tree = diagram();
    Tree::iterator c = kth(tree, 9);
    EXPECT_TRUE(c == tree.end());
    EXPECT_EQ(c.start(), (CountSum{8, 31}));
}

// Prefix sums of the diagram: 3, 4, 8, 9, 14, 23, 25, 31.
TEST(SumTreeTest, SeekBySumFindsWhereTheRunningSumReachesTheTarget) {
    Tree tree = diagram();
    EXPECT_EQ(sum_reaches(tree, 3)->value, 3);  // A, the very first item.
    EXPECT_EQ(sum_reaches(tree, 3).start(), (CountSum{0, 0}));
    EXPECT_EQ(sum_reaches(tree, 15)->value, 9);  // F: 14 before it, 23 through it.
    EXPECT_EQ(sum_reaches(tree, 15).start(), (CountSum{5, 14}));
    EXPECT_EQ(sum_reaches(tree, 31)->value, 6);  // H, reached exactly.
    EXPECT_EQ(sum_reaches(tree, 31).start(), (CountSum{7, 25}));
    EXPECT_TRUE(sum_reaches(tree, 32) == tree.end());
}

TEST(SumTreeTest, IteratorWalksTheSequenceWithRunningSummaries) {
    Tree tree = diagram();
    std::vector<int> items;
    std::vector<CountSum> starts;
    for (Tree::iterator c = tree.begin(); c != tree.end(); ++c) {
        items.push_back(c->value);
        starts.push_back(c.start());
    }
    EXPECT_EQ(items, (std::vector<int>{3, 1, 4, 1, 5, 9, 2, 6}));
    EXPECT_EQ(starts, (std::vector<CountSum>{
                          {0, 0}, {1, 3}, {2, 4}, {3, 8}, {4, 9}, {5, 14}, {6, 23}, {7, 25}}));
    EXPECT_EQ(tree.end().start(), (CountSum{8, 31}));
}

static_assert(std::forward_iterator<Tree::iterator>);

TEST(SumTreeTest, RangeForVisitsEveryItemInOrder) {
    Tree tree = diagram();
    std::vector<int> items;
    for (const Number& n : tree) items.push_back(n.value);
    EXPECT_EQ(items, (std::vector<int>{3, 1, 4, 1, 5, 9, 2, 6}));
}

TEST(SumTreeTest, FormatsTheShapeOneNodePerLine) {
    EXPECT_EQ(std::format("{}", diagram()), "[(4, 9) (4, 22)]\n"
                                            "  [(2, 4) (2, 5)]\n"
                                            "    leaf [3 1]\n"
                                            "    leaf [4 1]\n"
                                            "  [(2, 14) (2, 8)]\n"
                                            "    leaf [5 9]\n"
                                            "    leaf [2 6]");
    EXPECT_EQ(std::format("{}", Tree()), "[]");
}

TEST(SumTreeTest, SvgDrawsOneCellPerEntryAndOneArrowPerChild) {
    std::string svg = to_svg(diagram());
    EXPECT_TRUE(svg.starts_with("<svg"));
    EXPECT_EQ(count_of(svg, "<rect"), 14);  // 2 + 4 + 8 entries.
    EXPECT_EQ(count_of(svg, "<line"), 6);   // 2 + 4 children.
    EXPECT_NE(svg.find("Whole tree: (8, 31)"), std::string::npos);
    EXPECT_TRUE(to_svg(Tree()).starts_with("<svg"));
}

TEST(SumTreeTest, InsertIntoEmptyTreeCreatesRootLeaf) {
    Tree tree;
    tree.insert(tree.end(), Number{7});
    EXPECT_TRUE(same_tree(tree.root(), leaf({7}).get()));
    EXPECT_EQ(tree.summary(), (CountSum{1, 7}));
    EXPECT_TRUE(tree.check_invariants());
}

TEST(SumTreeTest, AppendingTheDiagramItemsBuildsTheDiagram) {
    Tree tree;
    for (int v : {3, 1, 4, 1, 5, 9, 2, 6}) tree.insert(tree.end(), Number{v});
    EXPECT_TRUE(same_tree(tree.root(), diagram().root()));
    EXPECT_TRUE(tree.check_invariants());
}

TEST(SumTreeTest, InsertWithRoomRefreshesTheSummariesAbove) {
    Tree tree = diagram();
    tree.insert(kth(tree, 2), Number{8});  // Before B, into the leaf [3, 1].
    EXPECT_TRUE(same_tree(tree.root(), nonleaf(nonleaf(leaf({3, 8, 1}), leaf({4, 1})),
                                               nonleaf(leaf({5, 9}), leaf({2, 6})))
                                           .get()));
    EXPECT_EQ(tree.summary(), (CountSum{9, 39}));
    EXPECT_TRUE(tree.check_invariants());
}

TEST(SumTreeTest, InsertIntoFullLeafSplitsIt) {
    Tree tree =
        Tree(nonleaf(nonleaf(leaf({3, 8, 1}), leaf({4, 1})), nonleaf(leaf({5, 9}), leaf({2, 6}))));
    tree.insert(kth(tree, 3), Number{7});  // Before B again: [3, 8, 7, 1] overflows.
    EXPECT_TRUE(same_tree(tree.root(), nonleaf(nonleaf(leaf({3, 8}), leaf({7, 1}), leaf({4, 1})),
                                               nonleaf(leaf({5, 9}), leaf({2, 6})))
                                           .get()));
    EXPECT_TRUE(tree.check_invariants());
}

TEST(SumTreeTest, InsertSplitsUpToTheRoot) {
    Tree tree = Tree(nonleaf(leaf({1, 2, 3}), leaf({4, 5, 6}), leaf({7, 8, 9})));
    tree.insert(tree.end(), Number{10});
    EXPECT_TRUE(same_tree(tree.root(), nonleaf(nonleaf(leaf({1, 2, 3}), leaf({4, 5, 6})),
                                               nonleaf(leaf({7, 8}), leaf({9, 10})))
                                           .get()));
    EXPECT_TRUE(tree.check_invariants());
}

TEST(SumTreeTest, EraseWithEnoughLeftRefreshesTheSummariesAbove) {
    Tree tree =
        Tree(nonleaf(nonleaf(leaf({3, 8, 1}), leaf({4, 1})), nonleaf(leaf({5, 9}), leaf({2, 6}))));
    tree.erase(kth(tree, 2));  // The 8.
    EXPECT_TRUE(same_tree(tree.root(), diagram().root()));
    EXPECT_EQ(tree.summary(), (CountSum{8, 31}));
    EXPECT_TRUE(tree.check_invariants());
}

TEST(SumTreeTest, EraseBorrowsFromTheLeftSibling) {
    Tree tree = Tree(nonleaf(leaf({1, 2, 3}), leaf({4, 5})));
    tree.erase(kth(tree, 5));
    EXPECT_TRUE(same_tree(tree.root(), nonleaf(leaf({1, 2}), leaf({3, 4})).get()));
    EXPECT_TRUE(tree.check_invariants());
}

TEST(SumTreeTest, EraseBorrowsFromTheRightSiblingWhenThereIsNoLeftOne) {
    Tree tree = Tree(nonleaf(leaf({1, 2}), leaf({3, 4, 5})));
    tree.erase(kth(tree, 1));
    EXPECT_TRUE(same_tree(tree.root(), nonleaf(leaf({2, 3}), leaf({4, 5})).get()));
    EXPECT_TRUE(tree.check_invariants());
}

TEST(SumTreeTest, EraseMergesWithTheLeftSibling) {
    Tree tree = Tree(nonleaf(leaf({1, 2}), leaf({3, 4}), leaf({5, 6})));
    tree.erase(kth(tree, 4));
    EXPECT_TRUE(same_tree(tree.root(), nonleaf(leaf({1, 2, 3}), leaf({5, 6})).get()));
    EXPECT_TRUE(tree.check_invariants());
}

TEST(SumTreeTest, EraseMergesWithTheRightSiblingWhenThereIsNoLeftOne) {
    Tree tree = Tree(nonleaf(leaf({1, 2}), leaf({3, 4}), leaf({5, 6})));
    tree.erase(kth(tree, 1));
    EXPECT_TRUE(same_tree(tree.root(), nonleaf(leaf({2, 3, 4}), leaf({5, 6})).get()));
    EXPECT_TRUE(tree.check_invariants());
}

// Erasing B merges its leaf with the next one, which leaves their parent with one child; the
// parent merges with its sibling in turn, and the root, down to one child, is dropped.
TEST(SumTreeTest, EraseMergesUpToTheRootAndShortensTheTree) {
    Tree tree = diagram();
    tree.erase(kth(tree, 2));
    EXPECT_TRUE(
        same_tree(tree.root(), nonleaf(leaf({3, 4, 1}), leaf({5, 9}), leaf({2, 6})).get()));
    EXPECT_EQ(tree.summary(), (CountSum{7, 30}));
    EXPECT_TRUE(tree.check_invariants());
}

TEST(SumTreeTest, ErasingEveryItemLeavesAnEmptyTree) {
    Tree tree;
    for (int v : {1, 2, 3, 4, 5}) tree.insert(tree.end(), Number{v});
    for (int i = 0; i < 5; i++) tree.erase(tree.begin());
    EXPECT_EQ(tree.root(), nullptr);
    EXPECT_EQ(tree.summary(), CountSum{});
    tree.insert(tree.end(), Number{6});
    EXPECT_EQ(values_of(tree.items()), (std::vector<int>{6}));
}

TEST(SumTreeTest, ReplaceUpdatesTheSummariesAndKeepsTheIteratorValid) {
    Tree tree = diagram();
    Tree::iterator c = kth(tree, 6);  // F, the 9.
    tree.replace(c, Number{10});
    EXPECT_EQ(tree.summary(), (CountSum{8, 32}));
    EXPECT_EQ(c->value, 10);
    EXPECT_EQ(c.start(), (CountSum{5, 14}));
    EXPECT_TRUE(tree.check_invariants());
}

TEST(SumTreeTest, CheckInvariantsRejectsStaleSummaries) {
    std::unique_ptr<Node> stale_child = nonleaf(leaf({1, 2}), leaf({3, 4}));
    stale_child->summary[0] = {2, 99};
    EXPECT_FALSE(Tree(std::move(stale_child)).check_invariants());

    std::unique_ptr<Node> stale_item = leaf({1, 2});
    stale_item->summary[1] = {1, 99};
    EXPECT_FALSE(Tree(std::move(stale_item)).check_invariants());
}

TEST(SumTreeTest, CheckInvariantsRejectsUnderfullNode) {
    EXPECT_FALSE(Tree(nonleaf(leaf({1}), leaf({2, 3}))).check_invariants());
}

// Inserting at random positions reads back as the vector built by the same inserts, and the tree
// stays valid throughout.
void InsertsAtRandomPositionsMatchVector(const std::vector<std::pair<int, int>>& ops) {
    Tree tree;
    std::vector<int> expected;
    for (auto [position, value] : ops) {
        int index = position % (static_cast<int>(expected.size()) + 1);
        tree.insert(kth(tree, index + 1), Number{value});
        expected.insert(expected.begin() + index, value);
        ASSERT_TRUE(tree.check_invariants()) << "insert " << value << " at " << index;
    }
    EXPECT_EQ(values_of(tree.items()), expected);
}
FUZZ_TEST(SumTreeFuzzTest, InsertsAtRandomPositionsMatchVector)
    .WithDomains(fuzztest::VectorOf(fuzztest::PairOf(fuzztest::InRange(0, 100),
                                                     fuzztest::InRange(-50, 50))));

// Random interleaved inserts and erases at random positions agree with a std::vector at every
// step, and the tree stays valid throughout.
template <typename AnyTree>
void interleaved_edits_match_vector(const std::vector<std::tuple<bool, int, int>>& ops) {
    AnyTree tree;
    std::vector<int> expected;
    for (auto [is_insert, position, value] : ops) {
        int size = static_cast<int>(expected.size());
        if (is_insert) {
            int index = position % (size + 1);
            tree.insert(kth(tree, index + 1), Number{value});
            expected.insert(expected.begin() + index, value);
        } else if (size > 0) {
            int index = position % size;
            tree.erase(kth(tree, index + 1));
            expected.erase(expected.begin() + index);
        }
        ASSERT_TRUE(tree.check_invariants()) << (is_insert ? "insert" : "erase");
        ASSERT_EQ(values_of(tree.items()), expected) << (is_insert ? "insert" : "erase");
    }
}

// At t = 2, where every few edits split or merge a node.
void InterleavedInsertsAndErasesMatchVector(const std::vector<std::tuple<bool, int, int>>& ops) {
    interleaved_edits_match_vector<Tree>(ops);
}
FUZZ_TEST(SumTreeFuzzTest, InterleavedInsertsAndErasesMatchVector)
    .WithDomains(fuzztest::VectorOf(fuzztest::TupleOf(fuzztest::Arbitrary<bool>(),
                                                      fuzztest::InRange(0, 100),
                                                      fuzztest::InRange(-50, 50))));

// At a larger degree, with positions spread wide enough for the tree to grow several levels.
void InterleavedInsertsAndErasesMatchVectorAtLargerDegree(
    const std::vector<std::tuple<bool, int, int>>& ops) {
    interleaved_edits_match_vector<SumTree<Number, 4>>(ops);
}
FUZZ_TEST(SumTreeFuzzTest, InterleavedInsertsAndErasesMatchVectorAtLargerDegree)
    .WithDomains(fuzztest::VectorOf(fuzztest::TupleOf(fuzztest::Arbitrary<bool>(),
                                                      fuzztest::InRange(0, 1000),
                                                      fuzztest::InRange(-50, 50))));

// With positive values the running sum is monotone, so a seek along it lands where a linear
// scan of the prefix sums does.
void SeekBySumMatchesLinearScan(const std::vector<int>& values, int target) {
    Tree tree;
    for (int v : values) tree.insert(tree.end(), Number{v});
    Tree::iterator found = sum_reaches(tree, target);

    int index = 0;
    int prefix = 0;
    int size = static_cast<int>(values.size());
    while (index < size && prefix + values[index] < target) prefix += values[index++];
    if (index == size) {
        EXPECT_TRUE(found == tree.end());
    } else {
        ASSERT_TRUE(found != tree.end());
        EXPECT_EQ(found->value, values[index]);
        EXPECT_EQ(found.start(), (CountSum{index, prefix}));
    }
}
FUZZ_TEST(SumTreeFuzzTest, SeekBySumMatchesLinearScan)
    .WithDomains(fuzztest::VectorOf(fuzztest::InRange(1, 20)).WithMaxSize(200),
                 fuzztest::InRange(0, 500));

}  // namespace btree
