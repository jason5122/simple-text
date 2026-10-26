#include "experiments/btree/bptree.h"

#include <array>
#include <climits>
#include <memory>
#include <utility>
#include <vector>

namespace bptree {

namespace {

// The nonleaf nodes from the root down to the parent of the node being worked on, which is how
// the book suggests implementing parent(N). A fixed array avoids a heap allocation on every
// insert and remove; with at least two children per nonleaf node, a tree of int keys is never
// more than 33 levels deep.
struct Path {
    std::array<Node*, 64> nodes;
    int depth = 0;
    void push(Node* x) { nodes[depth++] = x; }
    Node* pop() { return nodes[--depth]; }
};

// The descent shared by find (Figure 14.11), findRange (Figure 14.12), insert (Figure 14.16),
// and delete (Figure 14.21): walks from the root to the leaf that would contain v, recording the
// nonleaf nodes visited in `path` if one is given.
//
// The book's three cases, v below some Ki, v equal to Ki, and v above every key, all follow the
// child after the last key that is <= v: a key equal to v is found under Pi+1, never under Pi.
Node* find_leaf(Node* root, int v, Path* path = nullptr) {
    Node* C = root;
    while (!C->leaf) {
        if (path != nullptr) path->push(C);
        int i = 0;
        while (i < C->n && v >= C->key[i]) i++;
        C = C->c[i].get();
    }
    return C;
}

// The leaf half of find (Figure 14.11): "if for some i, Ki = v then return Pi else return null".
// The book returns the record pointer Pi; this tree stores no records, so a bool stands in.
bool leaf_contains(const Node& C, int v) {
    for (int i = 0; i < C.n; i++) {
        if (C.key[i] == v) return true;
    }
    return false;
}

// Figure 14.17, procedure insert_in_leaf(L, K, P). The book's two cases, before K1 or just after
// the highest Ki <= K, are both "shift the larger keys right by one and put K in the gap".
void insert_in_leaf(Node& L, int K) {
    int i = L.n;
    while (i > 0 && K < L.key[i - 1]) {
        L.key[i] = L.key[i - 1];
        i--;
    }
    L.key[i] = K;
    L.n++;
}

// "insert (K′, N′) in P just after N" (Figure 14.17).
void insert_after(Node& P, const Node* N, int K_prime, std::unique_ptr<Node> N_prime) {
    int i = 0;
    while (P.c[i].get() != N) i++;
    // Key j sits between c[j] and c[j + 1], so K′ lands at key[i] and N′ at c[i + 1] once the
    // entries after N have shifted right by one.
    for (int j = P.n; j > i; j--) {
        P.key[j] = P.key[j - 1];
        P.c[j + 1] = std::move(P.c[j]);
    }
    P.key[i] = K_prime;
    P.c[i + 1] = std::move(N_prime);
    P.n++;
}

// Figure 14.17, procedure insert_in_parent(N, K′, N′): node N was just split into N and N′, with
// K′ the least value in N′. `path` holds the nodes from the root down to parent(N), as recorded
// by find_leaf, and is consumed one level per split on the way up.
void insert_in_parent(
    std::unique_ptr<Node>& root, Path& path, Node* N, int K_prime, std::unique_ptr<Node> N_prime) {
    if (N == root.get()) {
        // Create a new node R containing N, K′, N′, and make R the root of the tree.
        auto R = std::make_unique<Node>();
        R->leaf = false;
        R->n = 1;
        R->key[0] = K_prime;
        R->c[0] = std::move(root);
        R->c[1] = std::move(N_prime);
        root = std::move(R);
        return;
    }
    Node* P = path.pop();  // Let P = parent(N).
    insert_after(*P, N, K_prime, std::move(N_prime));
    if (P->n == 2 * t) {
        // P had 2t pointers already and now holds one more than it may: split it. The book copies
        // P into a block of memory T with room for the extra entry; here that room is the node's
        // spare slot, so T is P itself. Create node P′.
        auto P_prime = std::make_unique<Node>();
        P_prime->leaf = false;
        // Let K′′ = T.Kt+1: the key between the pointers that stay and those that move.
        int K_double_prime = P->key[t];
        // Move T.Pt+2 … T.P2t+1 into P′, with the t − 1 keys that lie between those pointers.
        for (int i = t + 1; i <= 2 * t; i++) P_prime->c[i - t - 1] = std::move(P->c[i]);
        for (int i = t + 1; i < 2 * t; i++) P_prime->key[i - t - 1] = P->key[i];
        P_prime->n = t - 1;
        // T.P1 … T.Pt+1 stay in P, with the t keys between those pointers. The book keeps
        // ⌈(n+1)/2⌉ pointers, which is t + 1 for n = 2t.
        P->n = t;
        insert_in_parent(root, path, P, K_double_prime, std::move(P_prime));
    }
}

// "delete (K, P) from N" (Figure 14.21). A leaf loses the key K. A nonleaf node loses the key K
// and the pointer P that follows it (the book notes that P precedes K in a leaf entry but follows
// it in a nonleaf one), and the node P owned dies with it: that is the book's "delete node N".
void delete_from(Node& N, int K, const Node* P) {
    std::unique_ptr<Node> removed;
    int i = 0;
    if (N.leaf) {
        while (N.key[i] != K) i++;
    } else {
        while (N.c[i + 1].get() != P) i++;
        removed = std::move(N.c[i + 1]);
    }
    // Shift the entries after it left by one.
    for (int j = i; j < N.n - 1; j++) {
        N.key[j] = N.key[j + 1];
        if (!N.leaf) N.c[j + 1] = std::move(N.c[j + 2]);
    }
    N.n--;
}

// Figure 14.21, procedure delete_entry(N, K, P). `path` holds the nodes from the root down to
// parent(N), as recorded by find_leaf, and is consumed one level per recursive call. P is the
// pointer that follows K in a nonleaf node; leaves hold only keys, so it is absent for them.
void delete_entry(
    std::unique_ptr<Node>& root, Path& path, Node* N, int K, const Node* P = nullptr) {
    delete_from(*N, K, P);
    if (N == root.get()) {
        if (!N->leaf && N->n == 0) {
            // N is the root and has only one remaining child: make the child of N the new root
            // of the tree and delete N.
            std::unique_ptr<Node> child = std::move(N->c[0]);
            root = std::move(child);
        } else if (N->leaf && N->n == 0) {
            // Not in the book: the last key is gone, so the tree is empty again.
            root = nullptr;
        }
        return;
    }
    // "N has too few values/pointers": fewer than t values in a leaf, or fewer than t pointers in
    // a nonleaf node. The book's bounds are ⌈(n−1)/2⌉ and ⌈n/2⌉, both t for n = 2t.
    int min_keys = N->leaf ? t : t - 1;
    if (N->n >= min_keys) return;

    // Let N′ be the previous or next child of parent(N), preferring the previous one, and K′ the
    // value between pointers N and N′ in parent(N).
    Node* parent = path.pop();
    int j = 0;
    while (parent->c[j].get() != N) j++;
    bool N_prime_is_predecessor = j > 0;
    Node* N_prime = parent->c[N_prime_is_predecessor ? j - 1 : j + 1].get();
    int separator = N_prime_is_predecessor ? j - 1 : j;  // K′ is parent->key[separator].
    int K_prime = parent->key[separator];

    if (N->n + N_prime->n + (N->leaf ? 0 : 1) <= 2 * t - 1) {
        // The entries in N and N′ fit in a single node: coalesce them into the left one. If N is
        // a predecessor of N′, swap_variables(N, N′) so that N′ is the one that survives.
        if (!N_prime_is_predecessor) std::swap(N, N_prime);
        if (!N->leaf) {
            // Append K′ and all pointers and values in N to N′.
            N_prime->key[N_prime->n] = K_prime;
            for (int i = 0; i < N->n; i++) N_prime->key[N_prime->n + 1 + i] = N->key[i];
            for (int i = 0; i <= N->n; i++) N_prime->c[N_prime->n + 1 + i] = std::move(N->c[i]);
            N_prime->n += N->n + 1;
        } else {
            // Append all (Ki, Pi) pairs in N to N′; set N′.Pn = N.Pn.
            for (int i = 0; i < N->n; i++) N_prime->key[N_prime->n + i] = N->key[i];
            N_prime->n += N->n;
            N_prime->next = N->next;
        }
        delete_entry(root, path, parent, K_prime, N);  // Also deletes node N.
    } else if (N_prime_is_predecessor) {
        // Redistribution: borrow the last entry of N′.
        if (!N->leaf) {
            // Remove (N′.Kn, N′.Pn+1), the last value and pointer, from N′. Insert (N′.Pn+1, K′)
            // as the first pointer and value in N, shifting the others right. Replace K′ in
            // parent(N) by N′.Kn.
            std::unique_ptr<Node> borrowed = std::move(N_prime->c[N_prime->n]);
            int K_up = N_prime->key[N_prime->n - 1];
            N_prime->n--;
            for (int i = N->n; i > 0; i--) N->key[i] = N->key[i - 1];
            for (int i = N->n + 1; i > 0; i--) N->c[i] = std::move(N->c[i - 1]);
            N->key[0] = K_prime;
            N->c[0] = std::move(borrowed);
            N->n++;
            parent->key[separator] = K_up;
        } else {
            // Remove (N′.Pn, N′.Kn), the last pair, from N′. Insert it as the first in N, shifting
            // the others right. Replace K′ in parent(N) by N′.Kn.
            int borrowed = N_prime->key[N_prime->n - 1];
            N_prime->n--;
            for (int i = N->n; i > 0; i--) N->key[i] = N->key[i - 1];
            N->key[0] = borrowed;
            N->n++;
            parent->key[separator] = borrowed;
        }
    } else {
        // Symmetric to the case above: N′ is the next child, so borrow its first entry.
        if (!N->leaf) {
            // Remove (N′.P1, N′.K1) from N′. Append (K′, N′.P1) as the last value and pointer in
            // N. Replace K′ in parent(N) by N′.K1.
            std::unique_ptr<Node> borrowed = std::move(N_prime->c[0]);
            int K_up = N_prime->key[0];
            for (int i = 0; i < N_prime->n - 1; i++) N_prime->key[i] = N_prime->key[i + 1];
            for (int i = 0; i < N_prime->n; i++) N_prime->c[i] = std::move(N_prime->c[i + 1]);
            N_prime->n--;
            N->key[N->n] = K_prime;
            N->c[N->n + 1] = std::move(borrowed);
            N->n++;
            parent->key[separator] = K_up;
        } else {
            // Remove (N′.P1, N′.K1) from N′. Append it as the last pair in N. Replace K′ in
            // parent(N) by the key that is now N′.K1.
            int borrowed = N_prime->key[0];
            for (int i = 0; i < N_prime->n - 1; i++) N_prime->key[i] = N_prime->key[i + 1];
            N_prime->n--;
            N->key[N->n] = borrowed;
            N->n++;
            parent->key[separator] = N_prime->key[0];
        }
    }
}

// Checks the B+-tree properties of §14.3.1 on the subtree rooted at x, whose keys must all lie
// in [lo, hi). Returns the subtree's height, or -1 on a violation, and appends x's leaves to
// `leaves` left to right so the caller can verify the leaf chain.
//   - A nonleaf node holds between t and 2t pointers; the root holds at least 2 unless it is a
//     leaf. A leaf holds between t and 2t−1 keys; a root leaf may hold as few as 1.
//   - Keys are unique and sorted within a node, and the subtree under Pi holds only keys in
//     [Ki−1, Ki).
//   - Pointer slots past the last child are null, and every leaf sits at the same depth.
int check_subtree(
    const Node* x, long long lo, long long hi, bool is_root, std::vector<const Node*>& leaves) {
    int min_keys;
    if (x->leaf) {
        min_keys = is_root ? 1 : t;
    } else {
        min_keys = (is_root ? 2 : t) - 1;  // At least 2 or t pointers.
    }
    if (x->n < min_keys || x->n > 2 * t - 1) {
        return -1;
    }
    for (int j = 0; j < x->n; j++) {
        if (x->key[j] < lo || x->key[j] >= hi) {
            return -1;
        }
        if (j > 0 && x->key[j - 1] >= x->key[j]) {
            return -1;
        }
    }
    for (int j = x->leaf ? 0 : x->n + 1; j <= 2 * t; j++) {
        if (x->c[j] != nullptr) {
            return -1;
        }
    }
    if (x->leaf) {
        leaves.push_back(x);
        return 0;
    }

    int height = -1;
    for (int j = 0; j <= x->n; j++) {
        if (x->c[j] == nullptr) {
            return -1;
        }
        long long child_lo = j == 0 ? lo : x->key[j - 1];
        long long child_hi = j == x->n ? hi : x->key[j];
        int child_height = check_subtree(x->c[j].get(), child_lo, child_hi, false, leaves);
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

// Figure 14.11, function find(v).
bool BPlusTree::contains(int k) const {
    return root_ != nullptr && leaf_contains(*find_leaf(root_.get(), k), k);
}

// Figure 14.12, function findRange(lb, ub): the keys of all records with lb <= key <= ub, in
// order. The pointer to the next leaf, which this figure writes as C.Pn+1, is `next`.
std::vector<int> BPlusTree::find_range(int lb, int ub) const {
    std::vector<int> result_set;
    if (root_ == nullptr) return result_set;
    const Node* C = find_leaf(root_.get(), lb);
    // Let i be the least value such that Ki >= lb, then walk the leaf chain from there until a
    // key passes ub or the chain ends.
    int i = 0;
    while (i < C->n && C->key[i] < lb) i++;
    for (; C != nullptr; C = C->next, i = 0) {
        for (; i < C->n; i++) {
            if (C->key[i] > ub) return result_set;
            result_set.push_back(C->key[i]);
        }
    }
    return result_set;
}

// Figure 14.16, procedure insert(K, P). The book assumes K is new; here a key already in its
// leaf is rejected instead, and the return value reports whether anything was inserted.
bool BPlusTree::insert(int k) {
    // If the tree is empty, create an empty leaf node L, which is also the root.
    if (root_ == nullptr) root_ = std::make_unique<Node>();
    // Find the leaf node L that should contain key value k.
    Path path;
    Node* L = find_leaf(root_.get(), k, &path);
    if (leaf_contains(*L, k)) return false;
    insert_in_leaf(*L, k);
    if (L->n == 2 * t) {
        // L had 2t − 1 key values already and now holds one more than it may: split it. The book
        // copies L's keys and k into a block of memory T with room for the extra key; here that
        // room is the node's spare slot, so T is L itself. Create L′.
        auto L_prime = std::make_unique<Node>();
        // Set L′.Pn = L.Pn; set L.Pn = L′.
        L_prime->next = L->next;
        L->next = L_prime.get();
        // Move T.Kt+1 … T.K2t into L′; T.K1 … T.Kt stay in L. The book keeps ⌈n/2⌉ keys, which is
        // t for n = 2t.
        for (int i = t; i < 2 * t; i++) L_prime->key[i - t] = L->key[i];
        L_prime->n = t;
        L->n = t;
        // Let K′ be the smallest key-value in L′.
        int K_prime = L_prime->key[0];
        insert_in_parent(root_, path, L, K_prime, std::move(L_prime));
    }
    return true;
}

// Figure 14.21, procedure delete(K, P), under the name remove because delete is a C++ keyword.
// The book assumes K is present; here an absent key is reported instead.
bool BPlusTree::remove(int k) {
    if (root_ == nullptr) return false;
    Path path;
    // Find the leaf node L that contains k.
    Node* L = find_leaf(root_.get(), k, &path);
    if (!leaf_contains(*L, k)) return false;
    delete_entry(root_, path, L, k);
    return true;
}

std::vector<int> BPlusTree::keys() const {
    std::vector<int> out;
    if (root_ == nullptr) return out;
    const Node* C = root_.get();
    while (!C->leaf) C = C->c[0].get();
    for (; C != nullptr; C = C->next) {
        for (int i = 0; i < C->n; i++) out.push_back(C->key[i]);
    }
    return out;
}

bool BPlusTree::check_invariants() const {
    if (root_ == nullptr) return true;
    std::vector<const Node*> leaves;
    if (check_subtree(root_.get(), LLONG_MIN, LLONG_MAX, true, leaves) < 0) {
        return false;
    }
    // The leaf chain must visit the leaves left to right and end at the last one.
    for (size_t i = 0; i < leaves.size(); i++) {
        const Node* expected_next = i + 1 < leaves.size() ? leaves[i + 1] : nullptr;
        if (leaves[i]->next != expected_next) {
            return false;
        }
    }
    return true;
}

}  // namespace bptree
