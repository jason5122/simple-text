#include "experiments/btree/btree.h"

#include <print>

int main() {
    btree::BTree tree;
    for (int k : {1, 2, 3, 4, 5}) {
        tree.insert(k);
    }

    std::println("contains(5) = {}", tree.contains(5));
    std::println("contains(6) = {}", tree.contains(6));
}
