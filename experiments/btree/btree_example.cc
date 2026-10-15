// Examples of the CLRS B-tree, with minimum degree t = 4: every node holds 3 to 7 keys, and
// the eighth key to land in a node splits it. Build and run:
//
//   bin/ninja -C out/debug btree_example && out/debug/btree_example

#include "experiments/btree/btree.h"

#include <print>
#include <string>
#include <vector>

namespace {

std::string join(const std::vector<int>& keys) {
    std::string out;
    for (int k : keys) out += (out.empty() ? "" : " ") + std::to_string(k);
    return out;
}

}  // namespace

int main() {
    btree::BTree tree;

    // Ten squares in scrambled order. The first seven fill the root; the eighth splits it, and
    // keys() reads them back in order whatever order they went in.
    for (int k : {36, 4, 81, 1, 64, 16, 100, 9, 49, 25}) tree.insert(k);
    std::println("keys in order: [{}]", join(tree.keys()));

    for (int k : {25, 26, 100, 0}) std::println("contains({}) = {}", k, tree.contains(k));

    // CLRS's B-tree admits equal keys: inserting one twice stores it twice.
    tree.insert(25);
    std::println("after inserting 25 again: [{}]", join(tree.keys()));

    // Ten thousand more keys, in a scrambled order, leave the tree valid: every node within its
    // key bounds, every leaf at the same depth.
    for (int i = 0; i < 10000; i++) tree.insert(i * 7919 % 10007);
    std::println("{} keys; invariants {}", tree.keys().size(),
                 tree.check_invariants() ? "hold" : "BROKEN");
}
