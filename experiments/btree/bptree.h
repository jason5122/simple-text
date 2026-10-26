#pragma once

#include <array>
#include <memory>
#include <vector>

namespace bptree {

constexpr int t = 2;

struct Node {
    bool leaf = true;
    int n = 0;  // Number of keys. A nonleaf node has n + 1 children.
    std::array<int, 2 * t - 1> key;
    std::array<std::unique_ptr<Node>, 2 * t> c;  // Nonleaf only.
    Node* next = nullptr;                        // Leaf only: the book's Pn.
};

class BPlusTree {
public:
    BPlusTree() = default;
    bool contains(int k) const;
    bool insert(int k);
    bool remove(int k);
    std::vector<int> find_range(int lb, int ub) const;

    // Debug use.
    explicit BPlusTree(std::unique_ptr<Node> root) : root_{std::move(root)} {}
    std::vector<int> keys() const;  // In-order keys.
    bool check_invariants() const;
    const Node* root() const { return root_.get(); }

private:
    std::unique_ptr<Node> root_;
};

}  // namespace bptree
