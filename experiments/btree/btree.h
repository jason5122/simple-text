#pragma once

#include <memory>
#include <vector>

namespace btree {

struct Node;

class BTree {
public:
    BTree();
    ~BTree();
    BTree(BTree&&);
    BTree& operator=(BTree&&);
    BTree(const BTree&) = delete;
    BTree& operator=(const BTree&) = delete;

    bool contains(int k) const;
    void insert(int k);
    std::vector<int> keys() const;  // In-order keys.
    bool check_invariants() const;

private:
    std::unique_ptr<Node> root_;
};

}  // namespace btree
