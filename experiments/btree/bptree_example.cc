// Examples of the B+-tree of Database System Concepts §14.3 at the book's fanout, n = 4: nodes
// hold up to three keys, and the tree is printed after each change so that splits, borrowing,
// and coalescing can be seen. Build and run:
//
//   bin/ninja -C out/debug bptree_example && out/debug/bptree_example

#include "experiments/btree/bptree.h"

#include <print>
#include <string>
#include <vector>

namespace {

using Tree = bptree::BPlusTree<4>;

std::string join(const std::vector<int>& keys) {
    std::string out;
    for (int k : keys) out += (out.empty() ? "" : " ") + std::to_string(k);
    return out;
}

// One node per line, indented by depth. Nonleaf keys are separators; leaf keys are the data.
void print(const bptree::Node<4>* node, int depth = 0) {
    std::string keys;
    for (int i = 0; i < node->m; i++) keys += (i ? " " : "") + std::to_string(node->key[i]);
    std::println("{}{}[{}]", std::string(depth * 2, ' '), node->leaf ? "leaf " : "", keys);
    if (node->leaf) return;
    for (int i = 0; i <= node->m; i++) print(node->c[i].get(), depth + 1);
}

}  // namespace

int main() {
    // The twelve instructors of the book's Figure 14.9, as ints in alphabetical order. Inserting
    // them in that order fills each leaf only halfway before the next split, as §14.3.4 notes;
    // the book's figure is a hand-built tree with fuller leaves.
    Tree tree;
    for (int k : {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120}) tree.insert(k);
    std::println("after inserting 10 to 120 in order:");
    print(tree.root());

    std::println("contains(60) = {}, contains(65) = {}", tree.contains(60), tree.contains(65));
    std::println("find_range(35, 75) = [{}]", join(tree.find_range(35, 75)));

    // Adams (5) goes before Brandt (10): the first leaf has room, so no split.
    tree.insert(5);
    std::println("after inserting 5:");
    print(tree.root());

    // Removing Srinivasan (110) leaves Wu (120) alone in its leaf, too few for n = 4, so that leaf
    // coalesces with its neighbour and the parent drops the separator 110. Removing Wu then only
    // shrinks the merged leaf.
    tree.remove(110);
    tree.remove(120);
    std::println("after removing 110 and 120:");
    print(tree.root());

    std::println("keys in order: [{}]; invariants {}", join(tree.keys()),
                 tree.check_invariants() ? "hold" : "BROKEN");
}
