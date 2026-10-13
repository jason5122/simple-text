#pragma once

#include <array>
#include <memory>
#include <vector>

namespace bptree {

// The maximum number of pointers in a node, the book's n; a node holds up to n - 1 search-key
// values. n = 4 matches the figures in Database System Concepts §14.3, so the examples there can
// be traced directly.
constexpr int n = 4;

// A B+-tree node (Figure 14.7). Leaf and nonleaf nodes share one layout: sorted keys, plus either
// child pointers (nonleaf) or a pointer to the next leaf (leaf; the book's Pn).
//
// Indices are 0-based: the book's Kj is key[j - 1] and its Pj is c[j - 1].
struct Node {
    bool leaf = true;
    int m = 0;  // Number of keys. A nonleaf node has m + 1 children.
    std::array<int, n - 1> key;
    std::array<std::unique_ptr<Node>, n> c;  // Nonleaf only.
    Node* next = nullptr;                    // Leaf only: the book's Pn.
};

// Keys are unique, as the book assumes throughout §14.3.
class BPlusTree {
public:
    BPlusTree() = default;
    // Adopts a hand-built tree, so tests can construct the book's example trees node by node.
    explicit BPlusTree(std::unique_ptr<Node> root);

    bool contains(int k) const;
    // The keys in [lb, ub], ascending.
    std::vector<int> find_range(int lb, int ub) const;
    // Returns false, leaving the tree unchanged, if k is already present.
    bool insert(int k);
    // Returns false, leaving the tree unchanged, if k is not present.
    bool remove(int k);

    std::vector<int> keys() const;  // Every key, ascending, read off the leaf chain.
    bool check_invariants() const;
    const Node* root() const { return root_.get(); }

private:
    std::unique_ptr<Node> root_;  // Null when the tree is empty.
};

}  // namespace bptree
