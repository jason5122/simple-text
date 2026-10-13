#pragma once

#include <array>
#include <climits>
#include <memory>
#include <utility>
#include <vector>

namespace bptree {

// A B+-tree node (Figure 14.7) in a tree whose nodes hold up to n pointers, the book's n, and
// n - 1 search-key values. Leaf and nonleaf nodes share one layout: sorted keys, plus either
// child pointers (nonleaf) or a pointer to the next leaf (leaf; the book's Pn).
//
// Indices are 0-based: the book's Kj is key[j - 1] and its Pj is c[j - 1].
template <int n>
struct Node {
    bool leaf = true;
    int m = 0;  // Number of keys. A nonleaf node has m + 1 children.
    std::array<int, n - 1> key;
    std::array<std::unique_ptr<Node>, n> c;  // Nonleaf only.
    Node* next = nullptr;                    // Leaf only: the book's Pn.
};

// A B+-tree of unique int keys, as the book assumes throughout §14.3, with fanout n. The book's
// figures use n = 4 and the tests trace them at that size; the default is tuned for speed in
// memory (see bptree_perftest.cc), where a wide node keeps the tree shallow.
template <int n = 32>
class BPlusTree {
public:
    static constexpr int fanout = n;

    BPlusTree() = default;
    // Adopts a hand-built tree, so tests can construct the book's example trees node by node.
    explicit BPlusTree(std::unique_ptr<Node<n>> root) : root_{std::move(root)} {}

    bool contains(int k) const;
    // The keys in [lb, ub], ascending.
    std::vector<int> find_range(int lb, int ub) const;
    // Returns false, leaving the tree unchanged, if k is already present.
    bool insert(int k);
    // Returns false, leaving the tree unchanged, if k is not present.
    bool remove(int k);

    std::vector<int> keys() const;  // Every key, ascending, read off the leaf chain.
    bool check_invariants() const;
    const Node<n>* root() const { return root_.get(); }

private:
    static int check_subtree(const Node<n>* x,
                             long long lo,
                             long long hi,
                             bool is_root,
                             std::vector<const Node<n>*>& leaves);

    std::unique_ptr<Node<n>> root_;  // Null when the tree is empty.
};

// The pseudocode of Database System Concepts §14.3, one function per figure, with the record
// pointers dropped since this tree stores keys only. The BPlusTree methods at the end of this
// file are thin wrappers over these.
namespace book {

// The nonleaf nodes from the root down to the parent of the node being worked on, which is how
// the book suggests implementing parent(N). A fixed array avoids a heap allocation on every
// insert and remove; with at least two children per nonleaf node, a tree of int keys is never
// more than 33 levels deep.
template <int n>
struct Path {
    std::array<Node<n>*, 64> nodes;
    int depth = 0;
    void push(Node<n>* x) { nodes[depth++] = x; }
    Node<n>* pop() { return nodes[--depth]; }
};

// The descent shared by find (Figure 14.11), findRange (Figure 14.12), insert (Figure 14.16),
// and delete (Figure 14.21): walks from the root to the leaf that would contain v, recording the
// nonleaf nodes visited in `path` if one is given.
//
// With the 0-based i below, the book's Ki, Pi, and Pi+1 are key[i], c[i], and c[i + 1].
template <int n>
Node<n>* find_leaf(Node<n>* root, int v, Path<n>* path = nullptr) {
    Node<n>* C = root;
    while (!C->leaf) {
        if (path != nullptr) path->push(C);
        // Let i = smallest number such that v <= C.Ki.
        int i = 0;
        while (i < C->m && v > C->key[i]) i++;
        if (i == C->m) {
            // There is no such i: follow Pm, the last non-null pointer in the node.
            C = C->c[C->m].get();
        } else if (v == C->key[i]) {
            C = C->c[i + 1].get();
        } else {
            C = C->c[i].get();  // v < C.Ki
        }
    }
    return C;
}

// The leaf half of find (Figure 14.11): "if for some i, Ki = v then return Pi else return null".
// Returns a pointer to the leaf entry with key v, standing in for the book's record pointer Pi.
template <int n>
const int* find_in_leaf(Node<n>* C, int v) {
    for (int i = 0; i < C->m; i++) {
        if (C->key[i] == v) return &C->key[i];
    }
    return nullptr;  // No record with key value v exists.
}

// Figure 14.11, function find(v). Assumes a non-null root.
template <int n>
const int* find(Node<n>* root, int v) {
    return find_in_leaf(find_leaf(root, v), v);
}

// Figure 14.12, function findRange(lb, ub): the keys of all records with lb <= key <= ub, in
// order. The pointer to the next leaf, which this figure writes as C.Pn+1, is `next`.
template <int n>
std::vector<int> find_range(Node<n>* root, int lb, int ub) {
    std::vector<int> result_set;
    Node<n>* C = find_leaf(root, lb);
    // C is a leaf node. Let i be the least value such that Ki >= lb. If there is no such i, the
    // loop leaves i past the last key, which forces the move to the next leaf below.
    int i = 0;
    while (i < C->m && C->key[i] < lb) i++;
    bool done = false;
    while (!done) {
        if (i < C->m && C->key[i] <= ub) {
            result_set.push_back(C->key[i]);  // Add C.Pi to resultSet.
            i++;
        } else if (i < C->m && C->key[i] > ub) {
            done = true;
        } else if (i >= C->m && C->next != nullptr) {
            C = C->next;  // Move to next leaf.
            i = 0;
        } else {
            done = true;  // No more leaves to the right.
        }
    }
    return result_set;
}

// "A block of memory T" (Figures 14.16 and 14.17): room for one entry more than a node, so a
// full node plus the entry being inserted can be laid out in order and then split in two.
template <int n>
struct Block {
    int m = 0;
    std::array<int, n> key;
    std::array<std::unique_ptr<Node<n>>, n + 1> c;
};

// Figure 14.17, procedure insert_in_leaf(L, K, P). L is a leaf Node or the block T. The book's
// two cases, before K1 or just after the highest Ki <= K, are both "shift the larger keys right
// by one and put K in the gap".
template <typename NodeLike>
void insert_in_leaf(NodeLike& L, int K) {
    int i = L.m;
    while (i > 0 && K < L.key[i - 1]) {
        L.key[i] = L.key[i - 1];
        i--;
    }
    L.key[i] = K;
    L.m++;
}

// "insert (K′, N′) in P just after N" (Figure 14.17). P is a nonleaf Node or the block T.
template <int n, typename NodeLike>
void insert_after(NodeLike& P, const Node<n>* N, int K_prime, std::unique_ptr<Node<n>> N_prime) {
    int i = 0;
    while (P.c[i].get() != N) i++;
    // Key j sits between c[j] and c[j + 1], so K′ lands at key[i] and N′ at c[i + 1] once the
    // entries after N have shifted right by one.
    for (int j = P.m; j > i; j--) {
        P.key[j] = P.key[j - 1];
        P.c[j + 1] = std::move(P.c[j]);
    }
    P.key[i] = K_prime;
    P.c[i + 1] = std::move(N_prime);
    P.m++;
}

// Figure 14.17, procedure insert_in_parent(N, K′, N′): node N was just split into N and N′, with
// K′ the least value in N′. `path` holds the nodes from the root down to parent(N), as recorded
// by find_leaf, and is consumed one level per split on the way up.
template <int n>
void insert_in_parent(std::unique_ptr<Node<n>>& root,
                      Path<n>& path,
                      Node<n>* N,
                      int K_prime,
                      std::unique_ptr<Node<n>> N_prime) {
    if (N == root.get()) {
        // Create a new node R containing N, K′, N′, and make R the root of the tree.
        auto R = std::make_unique<Node<n>>();
        R->leaf = false;
        R->m = 1;
        R->key[0] = K_prime;
        R->c[0] = std::move(root);
        R->c[1] = std::move(N_prime);
        root = std::move(R);
        return;
    }
    Node<n>* P = path.pop();  // Let P = parent(N).
    if (P->m + 1 < n) {
        // P has less than n pointers.
        insert_after(*P, N, K_prime, std::move(N_prime));
    } else {
        // Split P. Copy P to T, and insert (K′, N′) into T just after N.
        Block<n> T;
        for (int i = 0; i < P->m; i++) T.key[i] = P->key[i];
        for (int i = 0; i <= P->m; i++) T.c[i] = std::move(P->c[i]);
        T.m = P->m;
        insert_after(T, N, K_prime, std::move(N_prime));
        // Erase all entries from P; create node P′.
        P->m = 0;
        auto P_prime = std::make_unique<Node<n>>();
        P_prime->leaf = false;
        // Copy T.P1 … T.P⌈(n+1)/2⌉ into P, with the keys that lie between those pointers.
        constexpr int half = (n + 2) / 2;  // ⌈(n + 1) / 2⌉
        for (int i = 0; i < half; i++) P->c[i] = std::move(T.c[i]);
        for (int i = 0; i < half - 1; i++) P->key[i] = T.key[i];
        P->m = half - 1;
        // Let K′′ = T.K⌈(n+1)/2⌉: the key between the pointers that stay and those that move.
        int K_double_prime = T.key[half - 1];
        // Copy T.P⌈(n+1)/2⌉+1 … T.Pn+1 into P′, with the keys that lie between those pointers.
        for (int i = half; i <= n; i++) P_prime->c[i - half] = std::move(T.c[i]);
        for (int i = half; i < n; i++) P_prime->key[i - half] = T.key[i];
        P_prime->m = n - half;
        insert_in_parent(root, path, P, K_double_prime, std::move(P_prime));
    }
}

// Figure 14.16, procedure insert(K, P). The book assumes K is new; here a K already in its leaf
// is rejected instead, and the return value reports whether anything was inserted.
template <int n>
bool insert(std::unique_ptr<Node<n>>& root, int K) {
    Node<n>* L;
    Path<n> path;
    if (root == nullptr) {
        // The tree is empty: create an empty leaf node L, which is also the root.
        root = std::make_unique<Node<n>>();
        L = root.get();
    } else {
        // Find the leaf node L that should contain key value K.
        L = find_leaf(root.get(), K, &path);
        if (find_in_leaf(L, K) != nullptr) return false;
    }
    if (L->m < n - 1) {
        insert_in_leaf(*L, K);
    } else {
        // L has n − 1 key values already, split it. Create L′, copy L's keys to T, and insert K
        // into T.
        auto L_prime = std::make_unique<Node<n>>();
        Block<n> T;
        for (int i = 0; i < L->m; i++) T.key[i] = L->key[i];
        T.m = L->m;
        insert_in_leaf(T, K);
        // Set L′.Pn = L.Pn; set L.Pn = L′.
        L_prime->next = L->next;
        L->next = L_prime.get();
        // Erase L.P1 through L.Kn−1 from L, then copy T.K1 … T.K⌈n/2⌉ into L and
        // T.K⌈n/2⌉+1 … T.Kn into L′.
        L->m = 0;
        constexpr int half = (n + 1) / 2;  // ⌈n / 2⌉
        for (int i = 0; i < half; i++) L->key[L->m++] = T.key[i];
        for (int i = half; i < n; i++) L_prime->key[L_prime->m++] = T.key[i];
        // Let K′ be the smallest key-value in L′.
        int K_prime = L_prime->key[0];
        insert_in_parent(root, path, L, K_prime, std::move(L_prime));
    }
    return true;
}

// "delete (K, P) from N" (Figure 14.21). A leaf loses the key K. A nonleaf node loses the key K
// and the pointer P that follows it (the book notes that P precedes K in a leaf entry but follows
// it in a nonleaf one), and the node P owned dies with it: that is the book's "delete node N".
template <int n>
void delete_from(Node<n>& N, int K, const Node<n>* P) {
    std::unique_ptr<Node<n>> removed;
    int i = 0;
    if (N.leaf) {
        while (N.key[i] != K) i++;
    } else {
        while (N.c[i + 1].get() != P) i++;
        removed = std::move(N.c[i + 1]);
    }
    // Shift the entries after it left by one.
    for (int j = i; j < N.m - 1; j++) {
        N.key[j] = N.key[j + 1];
        if (!N.leaf) N.c[j + 1] = std::move(N.c[j + 2]);
    }
    N.m--;
}

// "N has too few values/pointers" (Figure 14.21): fewer than ⌈(n−1)/2⌉ values in a leaf, or fewer
// than ⌈n/2⌉ pointers in a nonleaf node.
template <int n>
bool has_too_few(const Node<n>& N) {
    return N.leaf ? N.m < n / 2 : N.m + 1 < (n + 1) / 2;
}

// Figure 14.21, procedure delete_entry(N, K, P). `path` holds the nodes from the root down to
// parent(N), as recorded by find_leaf, and is consumed one level per recursive call. P is the
// pointer that follows K in a nonleaf node; leaves hold only keys, so it is absent for them.
template <int n>
void delete_entry(
    std::unique_ptr<Node<n>>& root, Path<n>& path, Node<n>* N, int K, const Node<n>* P = nullptr) {
    delete_from(*N, K, P);
    if (N == root.get()) {
        if (!N->leaf && N->m == 0) {
            // N is the root and has only one remaining child: make the child of N the new root
            // of the tree and delete N.
            std::unique_ptr<Node<n>> child = std::move(N->c[0]);
            root = std::move(child);
        } else if (N->leaf && N->m == 0) {
            // Not in the book: the last key is gone, so the tree is empty again.
            root = nullptr;
        }
        return;
    }
    if (!has_too_few(*N)) return;

    // Let N′ be the previous or next child of parent(N), preferring the previous one, and K′ the
    // value between pointers N and N′ in parent(N).
    Node<n>* parent = path.pop();
    int j = 0;
    while (parent->c[j].get() != N) j++;
    bool N_prime_is_predecessor = j > 0;
    Node<n>* N_prime = parent->c[N_prime_is_predecessor ? j - 1 : j + 1].get();
    int separator = N_prime_is_predecessor ? j - 1 : j;  // K′ is parent->key[separator].
    int K_prime = parent->key[separator];

    if (N->m + N_prime->m + (N->leaf ? 0 : 1) <= n - 1) {
        // The entries in N and N′ fit in a single node: coalesce them into the left one. If N is
        // a predecessor of N′, swap_variables(N, N′) so that N′ is the one that survives.
        if (!N_prime_is_predecessor) std::swap(N, N_prime);
        if (!N->leaf) {
            // Append K′ and all pointers and values in N to N′.
            N_prime->key[N_prime->m] = K_prime;
            for (int i = 0; i < N->m; i++) N_prime->key[N_prime->m + 1 + i] = N->key[i];
            for (int i = 0; i <= N->m; i++) N_prime->c[N_prime->m + 1 + i] = std::move(N->c[i]);
            N_prime->m += N->m + 1;
        } else {
            // Append all (Ki, Pi) pairs in N to N′; set N′.Pn = N.Pn.
            for (int i = 0; i < N->m; i++) N_prime->key[N_prime->m + i] = N->key[i];
            N_prime->m += N->m;
            N_prime->next = N->next;
        }
        delete_entry(root, path, parent, K_prime, N);  // Also deletes node N.
    } else if (N_prime_is_predecessor) {
        // Redistribution: borrow the last entry of N′.
        if (!N->leaf) {
            // Remove (N′.Km−1, N′.Pm), the last value and pointer, from N′. Insert (N′.Pm, K′) as
            // the first pointer and value in N, shifting the others right. Replace K′ in
            // parent(N) by N′.Km−1.
            std::unique_ptr<Node<n>> borrowed = std::move(N_prime->c[N_prime->m]);
            int K_up = N_prime->key[N_prime->m - 1];
            N_prime->m--;
            for (int i = N->m; i > 0; i--) N->key[i] = N->key[i - 1];
            for (int i = N->m + 1; i > 0; i--) N->c[i] = std::move(N->c[i - 1]);
            N->key[0] = K_prime;
            N->c[0] = std::move(borrowed);
            N->m++;
            parent->key[separator] = K_up;
        } else {
            // Remove (N′.Pm, N′.Km), the last pair, from N′. Insert it as the first in N, shifting
            // the others right. Replace K′ in parent(N) by N′.Km.
            int borrowed = N_prime->key[N_prime->m - 1];
            N_prime->m--;
            for (int i = N->m; i > 0; i--) N->key[i] = N->key[i - 1];
            N->key[0] = borrowed;
            N->m++;
            parent->key[separator] = borrowed;
        }
    } else {
        // Symmetric to the case above: N′ is the next child, so borrow its first entry.
        if (!N->leaf) {
            // Remove (N′.P1, N′.K1) from N′. Append (K′, N′.P1) as the last value and pointer in
            // N. Replace K′ in parent(N) by N′.K1.
            std::unique_ptr<Node<n>> borrowed = std::move(N_prime->c[0]);
            int K_up = N_prime->key[0];
            for (int i = 0; i < N_prime->m - 1; i++) N_prime->key[i] = N_prime->key[i + 1];
            for (int i = 0; i < N_prime->m; i++) N_prime->c[i] = std::move(N_prime->c[i + 1]);
            N_prime->m--;
            N->key[N->m] = K_prime;
            N->c[N->m + 1] = std::move(borrowed);
            N->m++;
            parent->key[separator] = K_up;
        } else {
            // Remove (N′.P1, N′.K1) from N′. Append it as the last pair in N. Replace K′ in
            // parent(N) by the key that is now N′.K1.
            int borrowed = N_prime->key[0];
            for (int i = 0; i < N_prime->m - 1; i++) N_prime->key[i] = N_prime->key[i + 1];
            N_prime->m--;
            N->key[N->m] = borrowed;
            N->m++;
            parent->key[separator] = N_prime->key[0];
        }
    }
}

// Figure 14.21, procedure delete(K, P), under the name remove because delete is a C++ keyword.
// The book assumes K is present; here an absent K is reported instead.
template <int n>
bool remove(std::unique_ptr<Node<n>>& root, int K) {
    if (root == nullptr) return false;
    Path<n> path;
    // Find the leaf node L that contains (K, P).
    Node<n>* L = find_leaf(root.get(), K, &path);
    if (find_in_leaf(L, K) == nullptr) return false;
    delete_entry(root, path, L, K);
    return true;
}

}  // namespace book

template <int n>
bool BPlusTree<n>::contains(int k) const {
    return root_ != nullptr && book::find(root_.get(), k) != nullptr;
}

template <int n>
std::vector<int> BPlusTree<n>::find_range(int lb, int ub) const {
    if (root_ == nullptr) return {};
    return book::find_range(root_.get(), lb, ub);
}

template <int n>
bool BPlusTree<n>::insert(int k) {
    return book::insert(root_, k);
}

template <int n>
bool BPlusTree<n>::remove(int k) {
    return book::remove(root_, k);
}

template <int n>
std::vector<int> BPlusTree<n>::keys() const {
    std::vector<int> out;
    if (root_ == nullptr) return out;
    const Node<n>* C = root_.get();
    while (!C->leaf) C = C->c[0].get();
    for (; C != nullptr; C = C->next) {
        for (int i = 0; i < C->m; i++) out.push_back(C->key[i]);
    }
    return out;
}

// Checks the B+-tree properties of §14.3.1 on the subtree rooted at x, whose keys must all lie
// in [lo, hi). Returns the subtree's height, or -1 on a violation, and appends x's leaves to
// `leaves` left to right so the caller can verify the leaf chain.
//   - A nonleaf node holds between ⌈n/2⌉ and n pointers; the root holds at least 2 unless it is
//     a leaf. A leaf holds between ⌈(n−1)/2⌉ and n−1 keys; a root leaf may hold as few as 1.
//   - Keys are unique and sorted within a node, and the subtree under Pi holds only keys in
//     [Ki−1, Ki).
//   - Pointer slots past the last child are null, and every leaf sits at the same depth.
template <int n>
int BPlusTree<n>::check_subtree(const Node<n>* x,
                                long long lo,
                                long long hi,
                                bool is_root,
                                std::vector<const Node<n>*>& leaves) {
    int min_keys;
    if (x->leaf) {
        min_keys = is_root ? 1 : n / 2;  // ⌈(n−1)/2⌉ = ⌊n/2⌋
    } else {
        min_keys = (is_root ? 2 : (n + 1) / 2) - 1;  // ⌈n/2⌉ pointers
    }
    if (x->m < min_keys || x->m > n - 1) {
        return -1;
    }
    for (int j = 0; j < x->m; j++) {
        if (x->key[j] < lo || x->key[j] >= hi) {
            return -1;
        }
        if (j > 0 && x->key[j - 1] >= x->key[j]) {
            return -1;
        }
    }
    for (int j = x->leaf ? 0 : x->m + 1; j < n; j++) {
        if (x->c[j] != nullptr) {
            return -1;
        }
    }
    if (x->leaf) {
        leaves.push_back(x);
        return 0;
    }

    int height = -1;
    for (int j = 0; j <= x->m; j++) {
        if (x->c[j] == nullptr) {
            return -1;
        }
        long long child_lo = j == 0 ? lo : x->key[j - 1];
        long long child_hi = j == x->m ? hi : x->key[j];
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

template <int n>
bool BPlusTree<n>::check_invariants() const {
    if (root_ == nullptr) return true;
    std::vector<const Node<n>*> leaves;
    if (check_subtree(root_.get(), LLONG_MIN, LLONG_MAX, true, leaves) < 0) {
        return false;
    }
    // The leaf chain must visit the leaves left to right and end at the last one.
    for (size_t i = 0; i < leaves.size(); i++) {
        const Node<n>* expected_next = i + 1 < leaves.size() ? leaves[i + 1] : nullptr;
        if (leaves[i]->next != expected_next) {
            return false;
        }
    }
    return true;
}

}  // namespace bptree
