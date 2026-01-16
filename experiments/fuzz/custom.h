#pragma once

#include <cassert>
#include <cstddef>

namespace editor {

struct BufferCursor {
    size_t line{}, column{};
};
enum class BufferType { Original, Mod };

struct Piece {
    BufferType type{};
    BufferCursor first{};
    BufferCursor last{};
    size_t length{};
    size_t lf_count{};
};

enum Color { RED, BLACK };

struct Node {
    Piece piece{};
    Color color{RED};
    Node* left{nullptr};
    Node* right{nullptr};
    Node* parent{nullptr};

    // Aggregates for order-statistics navigation.
    size_t subtree_length{0};
    size_t subtree_lf_count{0};
};

class RedBlackTree {
public:
    // Total document length and linefeeds.
    size_t length() const { return root ? root->subtree_length : 0; }
    size_t line_feed_count() const { return root ? root->subtree_lf_count : 0; }

    // Insert a piece at document offset 'at' (0..length()).
    void insert_at(size_t at, const Piece& p) {
        assert(at <= length());

        Node* z = new Node();
        z->piece = p;
        z->color = RED;
        z->left = z->right = z->parent = nullptr;
        z->subtree_length = p.length;
        z->subtree_lf_count = p.lf_count;

        Node* y = nullptr;
        Node* x = root;
        size_t base = 0;  // total offset of the current subtree's start

        // Descend by offset using left subtree lengths.
        while (x != nullptr) {
            y = x;

            size_t left_len = get_len(x->left);
            size_t split = base + left_len + x->piece.length;

            // Update aggregates on the path down (since we will insert under this node).
            // (You could also do this after insertion by walking parents; both are fine.)
            x->subtree_length += p.length;
            x->subtree_lf_count += p.lf_count;

            if (at < base + left_len + x->piece.length) {
                x = x->left;
            } else {
                base = base + left_len + x->piece.length;
                x = x->right;
            }
        }

        z->parent = y;
        if (y == nullptr) {
            root = z;
        } else {
            // Attach z under y by comparing against y's split.
            // We must recompute y's base to decide left vs right. Easiest: use navigation again:
            // Decide by whether 'at' is before y's "split point".
            size_t y_base = offset_of_subtree_start(y);
            size_t y_left = get_len(y->left);
            size_t y_split = y_base + y_left + y->piece.length;

            if (at < y_split) y->left = z;
            else y->right = z;
        }

        fix_insert(z);

        // Rotations may have messed aggregate values. Recompute on the local path to root.
        // This is cheap: O(log n).
        rebuild_upwards(z);
    }

    // Find node containing offset 'at' (0..length()-1). Returns nullptr if empty.
    Node* find_by_offset(size_t at) const {
        if (!root || at >= length()) return nullptr;

        Node* x = root;
        size_t base = 0;

        while (x) {
            size_t left_len = get_len(x->left);
            size_t start = base + left_len;
            size_t end = start + x->piece.length;

            if (at < start) {
                x = x->left;
            } else if (at >= end) {
                base = end;
                x = x->right;
            } else {
                return x;
            }
        }
        return nullptr;
    }

    // Removes the piece that contains offset 'at' (simple; for real editing you usually split
    // first).
    void erase_at(size_t at) {
        Node* z = find_by_offset(at);
        if (!z) return;
        delete_node(z);
    }

    ~RedBlackTree() { destroy(root); }

private:
    Node* root = nullptr;

    static size_t get_len(Node* n) { return n ? n->subtree_length : 0; }
    static size_t get_lf(Node* n) { return n ? n->subtree_lf_count : 0; }

    // Recompute aggregates from children and own piece.
    static void pull(Node* n) {
        if (!n) return;
        n->subtree_length = get_len(n->left) + n->piece.length + get_len(n->right);
        n->subtree_lf_count = get_lf(n->left) + n->piece.lf_count + get_lf(n->right);
    }

    // Walk to root pulling aggregates.
    static void rebuild_upwards(Node* n) {
        while (n) {
            pull(n);
            n = n->parent;
        }
    }

    // Compute the start offset of the subtree rooted at n by walking to root.
    // This is used only at attachment time above; you can avoid it by storing base during descent.
    size_t offset_of_subtree_start(Node* n) const {
        size_t offset = 0;
        Node* cur = n;
        while (cur && cur->parent) {
            Node* p = cur->parent;
            if (cur == p->right) {
                offset += get_len(p->left) + p->piece.length;
            }
            cur = p;
        }
        // Now offset is the distance from root start to n's subtree start,
        // but only if we started from root. This method assumes root start at 0.
        // That is fine for our usage inside insert_at.
        return offset;
    }

    void left_rotate(Node* x) {
        if (x == nullptr || x->right == nullptr) return;

        Node* y = x->right;
        x->right = y->left;
        if (y->left != nullptr) y->left->parent = x;

        y->parent = x->parent;
        if (x->parent == nullptr) root = y;
        else if (x == x->parent->left) x->parent->left = y;
        else x->parent->right = y;

        y->left = x;
        x->parent = y;

        // Fix aggregates locally.
        pull(x);
        pull(y);
    }

    void right_rotate(Node* y) {
        if (y == nullptr || y->left == nullptr) return;

        Node* x = y->left;
        y->left = x->right;
        if (x->right != nullptr) x->right->parent = y;

        x->parent = y->parent;
        if (y->parent == nullptr) root = x;
        else if (y == y->parent->left) y->parent->left = x;
        else y->parent->right = x;

        x->right = y;
        y->parent = x;

        // Fix aggregates locally.
        pull(y);
        pull(x);
    }

    void fix_insert(Node* z) {
        while (z != root && z->parent && z->parent->color == RED) {
            Node* p = z->parent;
            Node* g = p->parent;
            if (!g) break;

            if (p == g->left) {
                Node* y = g->right;
                if (y != nullptr && y->color == RED) {
                    p->color = BLACK;
                    y->color = BLACK;
                    g->color = RED;
                    z = g;
                } else {
                    if (z == p->right) {
                        z = p;
                        left_rotate(z);
                        p = z->parent;
                        g = p ? p->parent : nullptr;
                        if (!g) break;
                    }
                    p->color = BLACK;
                    g->color = RED;
                    right_rotate(g);
                }
            } else {
                Node* y = g->left;
                if (y != nullptr && y->color == RED) {
                    p->color = BLACK;
                    y->color = BLACK;
                    g->color = RED;
                    z = g;
                } else {
                    if (z == p->left) {
                        z = p;
                        right_rotate(z);
                        p = z->parent;
                        g = p ? p->parent : nullptr;
                        if (!g) break;
                    }
                    p->color = BLACK;
                    g->color = RED;
                    left_rotate(g);
                }
            }
        }
        if (root) root->color = BLACK;
    }

    void transplant(Node* u, Node* v) {
        if (u->parent == nullptr) root = v;
        else if (u == u->parent->left) u->parent->left = v;
        else u->parent->right = v;
        if (v != nullptr) v->parent = u->parent;
    }

    Node* minimum(Node* node) const {
        while (node && node->left != nullptr) node = node->left;
        return node;
    }

    void delete_node(Node* z) {
        if (z == nullptr) return;

        Node* y = z;
        Node* x = nullptr;
        Color y_original_color = y->color;

        Node* rebalance_from = z->parent;  // where we start pulling aggregates later

        if (z->left == nullptr) {
            x = z->right;
            transplant(z, z->right);
        } else if (z->right == nullptr) {
            x = z->left;
            transplant(z, z->left);
        } else {
            y = minimum(z->right);
            y_original_color = y->color;
            x = y->right;

            if (y->parent == z) {
                if (x != nullptr) x->parent = y;
            } else {
                transplant(y, y->right);
                y->right = z->right;
                if (y->right != nullptr) y->right->parent = y;
            }

            transplant(z, y);
            y->left = z->left;
            if (y->left != nullptr) y->left->parent = y;
            y->color = z->color;

            // Aggregates for y changed.
            pull(y);
            rebalance_from = y;
        }

        if (y_original_color == BLACK) fixDelete(x, rebalance_from);

        delete z;

        // Rebuild aggregates upward after structural changes.
        rebuild_upwards(rebalance_from ? rebalance_from : root);
    }

    // Deletion fixup that tolerates x == nullptr by carrying the parent separately.
    void fixDelete(Node* x, Node* parent) {
        while ((x != root) && (x == nullptr || x->color == BLACK)) {
            if (!parent) break;

            if (x == parent->left) {
                Node* w = parent->right;
                if (!w) {
                    x = parent;
                    parent = parent->parent;
                    continue;
                }

                if (w->color == RED) {
                    w->color = BLACK;
                    parent->color = RED;
                    left_rotate(parent);
                    w = parent->right;
                }

                if ((w->left == nullptr || w->left->color == BLACK) &&
                    (w->right == nullptr || w->right->color == BLACK)) {
                    w->color = RED;
                    x = parent;
                    parent = parent->parent;
                } else {
                    if (w->right == nullptr || w->right->color == BLACK) {
                        if (w->left != nullptr) w->left->color = BLACK;
                        w->color = RED;
                        right_rotate(w);
                        w = parent->right;
                    }
                    w->color = parent->color;
                    parent->color = BLACK;
                    if (w->right != nullptr) w->right->color = BLACK;
                    left_rotate(parent);
                    x = root;
                    break;
                }
            } else {
                Node* w = parent->left;
                if (!w) {
                    x = parent;
                    parent = parent->parent;
                    continue;
                }

                if (w->color == RED) {
                    w->color = BLACK;
                    parent->color = RED;
                    right_rotate(parent);
                    w = parent->left;
                }

                if ((w->right == nullptr || w->right->color == BLACK) &&
                    (w->left == nullptr || w->left->color == BLACK)) {
                    w->color = RED;
                    x = parent;
                    parent = parent->parent;
                } else {
                    if (w->left == nullptr || w->left->color == BLACK) {
                        if (w->right != nullptr) w->right->color = BLACK;
                        w->color = RED;
                        left_rotate(w);
                        w = parent->left;
                    }
                    w->color = parent->color;
                    parent->color = BLACK;
                    if (w->left != nullptr) w->left->color = BLACK;
                    right_rotate(parent);
                    x = root;
                    break;
                }
            }
        }
        if (x != nullptr) x->color = BLACK;
    }

    static void destroy(Node* n) {
        if (!n) return;
        destroy(n->left);
        destroy(n->right);
        delete n;
    }
};

}  // namespace editor
