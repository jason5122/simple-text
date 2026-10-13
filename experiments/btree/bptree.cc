#include "experiments/btree/bptree.h"

#include <array>
#include <climits>
#include <memory>
#include <utility>
#include <vector>

namespace bptree {

namespace {

// The pseudocode of Database System Concepts §14.3, one function per figure, with the record
// pointers dropped since this tree stores keys only. The BPlusTree methods below are thin
// wrappers over these.
namespace book {

// The descent shared by find (Figure 14.11), findRange (Figure 14.12), and insert (Figure
// 14.16): walks from the root to the leaf that would contain v. If `path` is given it receives
// the nonleaf nodes visited, root first, which is how the book suggests implementing parent(N).
//
// With the 0-based i below, the book's Ki, Pi, and Pi+1 are key[i], c[i], and c[i + 1].
Node* find_leaf(Node* root, int v, std::vector<Node*>* path = nullptr) {
    Node* C = root;
    while (!C->leaf) {
        if (path != nullptr) path->push_back(C);
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

// Figure 14.11, function find(v). Assumes a non-null root. Returns a pointer to the leaf entry
// with key v, standing in for the book's record pointer Pi, or null if no such entry exists.
const int* find(Node* root, int v) {
    Node* C = find_leaf(root, v);
    // C is a leaf node.
    for (int i = 0; i < C->m; i++) {
        if (C->key[i] == v) return &C->key[i];
    }
    return nullptr;  // No record with key value v exists.
}

// Figure 14.12, function findRange(lb, ub): the keys of all records with lb <= key <= ub, in
// order. The pointer to the next leaf, which this figure writes as C.Pn+1, is `next`.
std::vector<int> find_range(Node* root, int lb, int ub) {
    std::vector<int> result_set;
    Node* C = find_leaf(root, lb);
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
struct Block {
    int m = 0;
    std::array<int, n> key;
    std::array<std::unique_ptr<Node>, n + 1> c;
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
template <typename NodeLike>
void insert_after(NodeLike& P, const Node* N, int K_prime, std::unique_ptr<Node> N_prime) {
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
void insert_in_parent(std::unique_ptr<Node>& root,
                      std::vector<Node*>& path,
                      Node* N,
                      int K_prime,
                      std::unique_ptr<Node> N_prime) {
    if (N == root.get()) {
        // Create a new node R containing N, K′, N′, and make R the root of the tree.
        auto R = std::make_unique<Node>();
        R->leaf = false;
        R->m = 1;
        R->key[0] = K_prime;
        R->c[0] = std::move(root);
        R->c[1] = std::move(N_prime);
        root = std::move(R);
        return;
    }
    Node* P = path.back();  // Let P = parent(N).
    path.pop_back();
    if (P->m + 1 < n) {
        // P has less than n pointers.
        insert_after(*P, N, K_prime, std::move(N_prime));
    } else {
        // Split P. Copy P to T, and insert (K′, N′) into T just after N.
        Block T;
        for (int i = 0; i < P->m; i++) T.key[i] = P->key[i];
        for (int i = 0; i <= P->m; i++) T.c[i] = std::move(P->c[i]);
        T.m = P->m;
        insert_after(T, N, K_prime, std::move(N_prime));
        // Erase all entries from P; create node P′.
        P->m = 0;
        auto P_prime = std::make_unique<Node>();
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

// Figure 14.16, procedure insert(K, P). Assumes K is not already present.
void insert(std::unique_ptr<Node>& root, int K) {
    Node* L;
    std::vector<Node*> path;
    if (root == nullptr) {
        // The tree is empty: create an empty leaf node L, which is also the root.
        root = std::make_unique<Node>();
        L = root.get();
    } else {
        // Find the leaf node L that should contain key value K.
        L = find_leaf(root.get(), K, &path);
    }
    if (L->m < n - 1) {
        insert_in_leaf(*L, K);
    } else {
        // L has n − 1 key values already, split it. Create L′, copy L's keys to T, and insert K
        // into T.
        auto L_prime = std::make_unique<Node>();
        Block T;
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
    for (int j = i; j < N.m - 1; j++) {
        N.key[j] = N.key[j + 1];
        if (!N.leaf) N.c[j + 1] = std::move(N.c[j + 2]);
    }
    N.m--;
}

// "N has too few values/pointers" (Figure 14.21): fewer than ⌈(n−1)/2⌉ values in a leaf, or fewer
// than ⌈n/2⌉ pointers in a nonleaf node.
bool has_too_few(const Node& N) { return N.leaf ? N.m < n / 2 : N.m + 1 < (n + 1) / 2; }

// Figure 14.21, procedure delete_entry(N, K, P). `path` holds the nodes from the root down to
// parent(N), as recorded by find_leaf, and is consumed one level per recursive call.
void delete_entry(
    std::unique_ptr<Node>& root, std::vector<Node*>& path, Node* N, int K, const Node* P) {
    delete_from(*N, K, P);
    if (N == root.get()) {
        if (!N->leaf && N->m == 0) {
            // N is the root and has only one remaining child: make the child of N the new root
            // of the tree and delete N.
            std::unique_ptr<Node> child = std::move(N->c[0]);
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
    Node* parent = path.back();
    path.pop_back();
    int j = 0;
    while (parent->c[j].get() != N) j++;
    bool N_prime_is_predecessor = j > 0;
    Node* N_prime = parent->c[N_prime_is_predecessor ? j - 1 : j + 1].get();
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
            std::unique_ptr<Node> borrowed = std::move(N_prime->c[N_prime->m]);
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
            std::unique_ptr<Node> borrowed = std::move(N_prime->c[0]);
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
// Assumes K is present.
void remove(std::unique_ptr<Node>& root, int K) {
    std::vector<Node*> path;
    // Find the leaf node L that contains (K, P).
    Node* L = find_leaf(root.get(), K, &path);
    delete_entry(root, path, L, K, nullptr);
}

}  // namespace book

// Checks the B+-tree properties of §14.3.1 on the subtree rooted at x, whose keys must all lie
// in [lo, hi). Returns the subtree's height, or -1 on a violation, and appends x's leaves to
// `leaves` left to right so the caller can verify the leaf chain.
//   - A nonleaf node holds between ⌈n/2⌉ and n pointers; the root holds at least 2 unless it is
//     a leaf. A leaf holds between ⌈(n−1)/2⌉ and n−1 keys; a root leaf may hold as few as 1.
//   - Keys are unique and sorted within a node, and the subtree under Pi holds only keys in
//     [Ki−1, Ki).
//   - Pointer slots past the last child are null, and every leaf sits at the same depth.
int check_subtree(
    const Node* x, long long lo, long long hi, bool is_root, std::vector<const Node*>& leaves) {
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

}  // namespace

BPlusTree::BPlusTree(std::unique_ptr<Node> root) : root_{std::move(root)} {}

bool BPlusTree::contains(int k) const {
    return root_ != nullptr && book::find(root_.get(), k) != nullptr;
}

std::vector<int> BPlusTree::find_range(int lb, int ub) const {
    if (root_ == nullptr) return {};
    return book::find_range(root_.get(), lb, ub);
}

bool BPlusTree::insert(int k) {
    if (contains(k)) return false;
    book::insert(root_, k);
    return true;
}

bool BPlusTree::remove(int k) {
    if (!contains(k)) return false;
    book::remove(root_, k);
    return true;
}

std::vector<int> BPlusTree::keys() const {
    std::vector<int> out;
    if (root_ == nullptr) return out;
    const Node* C = root_.get();
    while (!C->leaf) C = C->c[0].get();
    for (; C != nullptr; C = C->next) {
        for (int i = 0; i < C->m; i++) out.push_back(C->key[i]);
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
