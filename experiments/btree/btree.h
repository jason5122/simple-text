#pragma once

#include <array>
#include <memory>
#include <vector>

namespace btree {

constexpr int t = 4;

struct Node {
    bool leaf = true;
    int n = 0;  // Number of keys. A nonleaf node has n + 1 children.
    std::array<int, 2 * t - 1> key;
    std::array<std::unique_ptr<Node>, 2 * t> c;  // Nonleaf only.
};

class BTree {
public:
    BTree();

    bool contains(int k) const;
    void insert(int k);

    // Debug use.
    std::vector<int> keys() const;  // In-order keys.
    bool check_invariants() const;

private:
    std::unique_ptr<Node> root_;
};

}  // namespace btree
