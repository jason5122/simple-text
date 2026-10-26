#include "experiments/btree/btree.h"

#include <climits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace btree {

namespace {

struct Position {
    Node* node;
    int index;
};

std::optional<Position> search(Node* x, int k) {
    int i = 0;
    while (i < x->n && k > x->key[i]) i++;

    if (i < x->n && k == x->key[i]) {
        return Position{x, i};
    } else if (x->leaf) {
        return std::nullopt;
    } else {
        return search(x->c[i].get(), k);
    }
}

void split_child(Node* x, int i) {
    auto z = std::make_unique<Node>();
    Node* y = x->c[i].get();
    z->leaf = y->leaf;
    z->n = t - 1;
    for (int j = 0; j < t - 1; j++) {
        z->key[j] = y->key[j + t];
    }
    if (!y->leaf) {
        for (int j = 0; j < t; j++) {
            z->c[j] = std::move(y->c[j + t]);
        }
    }
    y->n = t - 1;
    for (int j = x->n; j >= i + 1; j--) {
        x->c[j + 1] = std::move(x->c[j]);
    }
    x->c[i + 1] = std::move(z);
    for (int j = x->n - 1; j >= i; j--) {
        x->key[j + 1] = x->key[j];
    }
    x->key[i] = y->key[t - 1];
    x->n++;
}

void insert_nonfull(Node* x, int k) {
    int i = x->n - 1;
    if (x->leaf) {
        while (i >= 0 && k < x->key[i]) {
            x->key[i + 1] = x->key[i];
            i--;
        }
        x->key[i + 1] = k;
        x->n++;
    } else {
        while (i >= 0 && k < x->key[i]) {
            i--;
        }
        i++;
        if (x->c[i]->n == 2 * t - 1) {
            split_child(x, i);
            if (k > x->key[i]) i++;
        }
        insert_nonfull(x->c[i].get(), k);
    }
}

void collect_keys(const Node* x, std::vector<int>& out) {
    for (int i = 0; i < x->n; i++) {
        if (!x->leaf) {
            collect_keys(x->c[i].get(), out);
        }
        out.push_back(x->key[i]);
    }
    if (!x->leaf) {
        collect_keys(x->c[x->n].get(), out);
    }
}

// Checks the B-tree properties from CLRS 18.1 on the subtree rooted at x,
// whose keys must all lie in [lo, hi]. Returns the subtree's height, or -1 on
// a violation.
//   - Every node except the root holds at least t-1 keys; none holds more
//     than 2t-1.
//   - Keys are ordered within a node and bracketed by the parent's keys.
//     Bounds are inclusive because CLRS allows equal keys
//     (k1 <= key1 <= k2 <= ...) and insert() never rejects a duplicate.
//   - Every leaf sits at the same depth.
int check_subtree(const Node* x, int lo, int hi, bool is_root) {
    if (!is_root && x->n < t - 1) {
        return -1;
    }
    if (x->n > 2 * t - 1) {
        return -1;
    }
    for (int j = 0; j < x->n; j++) {
        if (x->key[j] < lo || x->key[j] > hi) {
            return -1;
        }
        if (j > 0 && x->key[j - 1] > x->key[j]) {
            return -1;
        }
    }
    if (x->leaf) {
        return 0;
    }

    int height = -1;
    for (int j = 0; j <= x->n; j++) {
        int child_lo = j == 0 ? lo : x->key[j - 1];
        int child_hi = j == x->n ? hi : x->key[j];
        int child_height = check_subtree(x->c[j].get(), child_lo, child_hi, false);
        if (child_height < 0) {
            return -1;
        }
        if (j == 0) {
            height = child_height;
        } else if (child_height != height) {
            return -1;
        }
    }
    return height + 1;
}

}  // namespace

BTree::BTree() : root_{std::make_unique<Node>()} {}

bool BTree::contains(int k) const { return search(root_.get(), k).has_value(); }

void BTree::insert(int k) {
    if (root_->n == 2 * t - 1) {
        auto s = std::make_unique<Node>();
        s->leaf = false;
        s->c[0] = std::move(root_);
        root_ = std::move(s);
        split_child(root_.get(), 0);
    }
    insert_nonfull(root_.get(), k);
}

std::vector<int> BTree::keys() const {
    std::vector<int> out;
    collect_keys(root_.get(), out);
    return out;
}

bool BTree::check_invariants() const {
    return check_subtree(root_.get(), INT_MIN, INT_MAX, true) >= 0;
}

}  // namespace btree
