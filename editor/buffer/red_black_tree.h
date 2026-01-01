#pragma once

#include <cassert>
#include <cstddef>

namespace editor {

struct BufferCursor {
    size_t line{};
    size_t column{};
    bool operator==(const BufferCursor&) const = default;
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

    // Cached left-subtree aggregates.
    size_t left_length{0};
    size_t left_lf_count{0};

    // Full subtree aggregates.
    size_t subtree_length{0};
    size_t subtree_lf_count{0};
};

class RedBlackTree {
public:
    RedBlackTree() = default;

    // Queries.
    bool empty() const { return root_ == nullptr; }
    explicit operator bool() const { return root_ != nullptr; }

    size_t length() const { return root_ ? root_->subtree_length : 0; }
    size_t line_feed_count() const { return root_ ? root_->subtree_lf_count : 0; }
    size_t left_length() const { return root_ ? root_->left_length : 0; }
    size_t left_line_feed_count() const { return root_ ? root_->left_lf_count : 0; }
    bool is_black() const { return !root_ || root_->color == BLACK; }
    bool is_red() const { return root_ && root_->color == RED; }

    // clang-format off
    const Piece& piece() const { assert(root_); return root_->piece; }
    Color color() const { assert(root_); return root_->color; }
    RedBlackTree left() const { assert(root_); return RedBlackTree(root_->left); }
    RedBlackTree right() const { assert(root_); return RedBlackTree(root_->right); }
    // clang-format on

    // Debug use.
    bool satisfies_red_black_invariants() const { return true; }  // TODO: implement if needed

    bool operator==(const RedBlackTree&) const = default;

    // Mutators (mutable RB tree).
    void insert(size_t at, const Piece& p) { insert_at(at, p); }
    void remove(size_t at) { erase_at(at); }

    void insert_at(size_t at, const Piece& p) {
        assert(at <= length());

        Node* z = new Node();
        z->piece = p;
        z->color = RED;
        z->left = z->right = z->parent = nullptr;

        // Initialize aggregates for leaf.
        z->left_length = 0;
        z->left_lf_count = 0;
        z->subtree_length = p.length;
        z->subtree_lf_count = p.lf_count;

        Node* y = nullptr;
        Node* x = root_;
        size_t base = 0;
        bool attach_left = false;

        // Descend by offset using cached left_length. Do NOT update aggregates here.
        while (x != nullptr) {
            y = x;

            size_t split = base + x->left_length + x->piece.length;
            if (at < split) {
                attach_left = true;
                x = x->left;
            } else {
                attach_left = false;
                base = split;
                x = x->right;
            }
        }

        z->parent = y;
        if (y == nullptr) {
            root_ = z;
        } else if (attach_left) {
            y->left = z;
        } else {
            y->right = z;
        }

        // Fix RB invariants (rotations pull local aggregates).
        fix_insert(z);

        // Recompute aggregates on the path to root.
        rebuild_upwards(z);
    }

    Node* find_by_offset(size_t at) const {
        if (!root_ || at >= length()) return nullptr;

        Node* x = root_;
        size_t base = 0;

        while (x) {
            size_t start = base + x->left_length;
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

    void erase_at(size_t at) {
        Node* z = find_by_offset(at);
        if (!z) return;
        delete_node(z);
    }

    // ~RedBlackTree() { destroy(root_); }

private:
    Node* root_ = nullptr;

    // Non-owning subtree-view constructor.
    explicit RedBlackTree(Node* root) : root_(root) {}

    static size_t get_len(Node* n) { return n ? n->subtree_length : 0; }
    static size_t get_lf(Node* n) { return n ? n->subtree_lf_count : 0; }

    static void pull(Node* n) {
        if (!n) return;
        n->left_length = get_len(n->left);
        n->left_lf_count = get_lf(n->left);
        n->subtree_length = get_len(n->left) + n->piece.length + get_len(n->right);
        n->subtree_lf_count = get_lf(n->left) + n->piece.lf_count + get_lf(n->right);
    }

    static void rebuild_upwards(Node* n) {
        while (n) {
            pull(n);
            n = n->parent;
        }
    }

    void left_rotate(Node* x) {
        if (!x || !x->right) return;

        Node* y = x->right;
        x->right = y->left;
        if (y->left) y->left->parent = x;

        y->parent = x->parent;
        if (!x->parent) root_ = y;
        else if (x == x->parent->left) x->parent->left = y;
        else x->parent->right = y;

        y->left = x;
        x->parent = y;

        // Fix aggregates locally after the structure change.
        pull(x);
        pull(y);
    }

    void right_rotate(Node* y) {
        if (!y || !y->left) return;

        Node* x = y->left;
        y->left = x->right;
        if (x->right) x->right->parent = y;

        x->parent = y->parent;
        if (!y->parent) root_ = x;
        else if (y == y->parent->left) y->parent->left = x;
        else y->parent->right = x;

        x->right = y;
        y->parent = x;

        pull(y);
        pull(x);
    }

    void fix_insert(Node* z) {
        while (z != root_ && z->parent && z->parent->color == RED) {
            Node* p = z->parent;
            Node* g = p->parent;
            if (!g) break;

            if (p == g->left) {
                Node* y = g->right;  // uncle
                if (y && y->color == RED) {
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
                Node* y = g->left;  // uncle
                if (y && y->color == RED) {
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
        if (root_) root_->color = BLACK;
    }

    void transplant(Node* u, Node* v) {
        if (!u->parent) root_ = v;
        else if (u == u->parent->left) u->parent->left = v;
        else u->parent->right = v;
        if (v) v->parent = u->parent;
    }

    Node* minimum(Node* node) const {
        while (node && node->left) node = node->left;
        return node;
    }

    // Deletion fixup that tolerates x == nullptr by carrying parent explicitly.
    void fixDelete(Node* x, Node* parent) {
        while ((x != root_) && (x == nullptr || x->color == BLACK)) {
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
                    if (!w) {
                        x = parent;
                        parent = parent->parent;
                        continue;
                    }
                }

                if ((!w->left || w->left->color == BLACK) &&
                    (!w->right || w->right->color == BLACK)) {
                    w->color = RED;
                    x = parent;
                    parent = parent->parent;
                } else {
                    if (!w->right || w->right->color == BLACK) {
                        if (w->left) w->left->color = BLACK;
                        w->color = RED;
                        right_rotate(w);
                        w = parent->right;
                        if (!w) {
                            x = parent;
                            parent = parent->parent;
                            continue;
                        }
                    }
                    w->color = parent->color;
                    parent->color = BLACK;
                    if (w->right) w->right->color = BLACK;
                    left_rotate(parent);
                    x = root_;
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
                    if (!w) {
                        x = parent;
                        parent = parent->parent;
                        continue;
                    }
                }

                if ((!w->right || w->right->color == BLACK) &&
                    (!w->left || w->left->color == BLACK)) {
                    w->color = RED;
                    x = parent;
                    parent = parent->parent;
                } else {
                    if (!w->left || w->left->color == BLACK) {
                        if (w->right) w->right->color = BLACK;
                        w->color = RED;
                        left_rotate(w);
                        w = parent->left;
                        if (!w) {
                            x = parent;
                            parent = parent->parent;
                            continue;
                        }
                    }
                    w->color = parent->color;
                    parent->color = BLACK;
                    if (w->left) w->left->color = BLACK;
                    right_rotate(parent);
                    x = root_;
                    break;
                }
            }
        }
        if (x) x->color = BLACK;
    }

    void delete_node(Node* z) {
        if (!z) return;

        Node* y = z;
        Node* x = nullptr;
        Node* x_parent = z->parent;
        Color y_original_color = y->color;

        if (!z->left) {
            x = z->right;
            x_parent = z->parent;
            transplant(z, z->right);
        } else if (!z->right) {
            x = z->left;
            x_parent = z->parent;
            transplant(z, z->left);
        } else {
            y = minimum(z->right);
            y_original_color = y->color;
            x = y->right;

            if (y->parent == z) {
                x_parent = y;
                if (x) x->parent = y;
            } else {
                x_parent = y->parent;
                transplant(y, y->right);
                y->right = z->right;
                if (y->right) y->right->parent = y;
            }

            transplant(z, y);
            y->left = z->left;
            if (y->left) y->left->parent = y;
            y->color = z->color;

            // y now has different children; pull it.
            pull(y);
        }

        if (y_original_color == BLACK) {
            fixDelete(x, x_parent);
        }

        // Fix aggregates from the nearest affected parent to root.
        rebuild_upwards(x_parent ? x_parent : root_);

        delete z;
    }

    static void destroy(Node* n) {
        if (!n) return;
        destroy(n->left);
        destroy(n->right);
        delete n;
    }
};

}  // namespace editor
