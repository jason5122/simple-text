#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

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

class NodeArena {
public:
    explicit NodeArena(size_t block_bytes = 1 << 20) : block_bytes_(block_bytes) {}

    NodeArena(const NodeArena&) = delete;
    NodeArena& operator=(const NodeArena&) = delete;

    template <class T, class... Args>
    T* make(Args&&... args) {
        void* mem = alloc(sizeof(T), alignof(T));
        return ::new (mem) T(std::forward<Args>(args)...);
    }

    // Optional: reset whole arena (drops all nodes).
    void reset() {
        blocks_.clear();
        cur_ = end_ = nullptr;
    }

private:
    void* alloc(size_t bytes, size_t align) {
        uintptr_t p = reinterpret_cast<uintptr_t>(cur_);
        uintptr_t aligned = (p + (align - 1)) & ~(uintptr_t(align - 1));
        uintptr_t next = aligned + bytes;

        if (cur_ == nullptr || next > reinterpret_cast<uintptr_t>(end_)) {
            // new block
            size_t cap = block_bytes_;
            if (cap < bytes + align) cap = bytes + align;
            blocks_.push_back(std::make_unique<std::byte[]>(cap));
            cur_ = blocks_.back().get();
            end_ = cur_ + cap;

            p = reinterpret_cast<uintptr_t>(cur_);
            aligned = (p + (align - 1)) & ~(uintptr_t(align - 1));
            next = aligned + bytes;
        }

        cur_ = reinterpret_cast<std::byte*>(next);
        return reinterpret_cast<void*>(aligned);
    }

    size_t block_bytes_;
    std::vector<std::unique_ptr<std::byte[]>> blocks_;
    std::byte* cur_ = nullptr;
    std::byte* end_ = nullptr;
};

class RedBlackTree {
public:
    enum class Color : uint8_t { Red, Black };

    RedBlackTree() = default;

    explicit operator bool() const { return node_ != nullptr; }
    bool empty() const { return node_ == nullptr; }

    size_t length() const { return node_ ? node_->data.subtree_length : 0; }
    size_t line_feed_count() const { return node_ ? node_->data.subtree_lf_count : 0; }
    size_t left_length() const { return node_ ? node_->data.left_length : 0; }
    size_t left_line_feed_count() const { return node_ ? node_->data.left_lf_count : 0; }

    const Piece& piece() const {
        assert(node_);
        return node_->data.piece;
    }
    Color color() const { return node_ ? node_->color : Color::Black; }  // null is black
    RedBlackTree left() const {
        assert(node_);
        return RedBlackTree(node_->left);
    }
    RedBlackTree right() const {
        assert(node_);
        return RedBlackTree(node_->right);
    }

    // Persistent insert: returns new root, allocates O(log n) nodes from arena.
    RedBlackTree insert(NodeArena& A, size_t at, const Piece& p) const {
        RedBlackTree t = ins(A, *this, at, p, 0);
        return t.blacken(A);  // no-op if already black
    }

private:
    struct NodeData {
        Piece piece{};
        size_t left_length{};
        size_t left_lf_count{};
        size_t subtree_length{};
        size_t subtree_lf_count{};
    };

    struct Node {
        Color color;
        const Node* left;
        NodeData data;
        const Node* right;
        Node(Color c, const Node* l, NodeData d, const Node* r)
            : color(c), left(l), data(d), right(r) {}
    };

    const Node* node_ = nullptr;
    explicit RedBlackTree(const Node* n) : node_(n) {}

    static NodeData make_data(const RedBlackTree& left,
                              const Piece& p,
                              const RedBlackTree& right) {
        NodeData d;
        d.piece = p;
        d.left_length = left.length();
        d.left_lf_count = left.line_feed_count();
        d.subtree_length = left.length() + p.length + right.length();
        d.subtree_lf_count = left.line_feed_count() + p.lf_count + right.line_feed_count();
        return d;
    }

    static RedBlackTree make(
        NodeArena& A, Color c, const RedBlackTree& l, const Piece& p, const RedBlackTree& r) {
        NodeData d = make_data(l, p, r);
        const Node* n = A.make<Node>(c, l.node_, d, r.node_);
        return RedBlackTree(n);
    }

    bool is_red() const { return node_ && node_->color == Color::Red; }
    bool is_black() const { return !node_ || node_->color == Color::Black; }

    bool double_red_left() const { return is_red() && left().is_red(); }
    bool double_red_right() const { return is_red() && right().is_red(); }

    RedBlackTree blacken(NodeArena& A) const {
        if (!node_ || node_->color == Color::Black) return *this;
        return make(A, Color::Black, left(), piece(), right());
    }

    // Okasaki-style balancing. Allocates only when a rotation/recolor happens.
    RedBlackTree balance(NodeArena& A) const {
        if (!node_ || is_red()) return *this;

        RedBlackTree l = left();
        RedBlackTree r = right();
        const Piece& p = piece();

        if (l.double_red_left()) {
            return make(A, Color::Red, l.left().blacken(A), l.piece(),
                        make(A, Color::Black, l.right(), p, r));
        }
        if (l.double_red_right()) {
            return make(A, Color::Red,
                        make(A, Color::Black, l.left(), l.piece(), l.right().left()),
                        l.right().piece(), make(A, Color::Black, l.right().right(), p, r));
        }
        if (r.double_red_left()) {
            return make(A, Color::Red, make(A, Color::Black, l, p, r.left().left()),
                        r.left().piece(),
                        make(A, Color::Black, r.left().right(), r.piece(), r.right()));
        }
        if (r.double_red_right()) {
            return make(A, Color::Red, make(A, Color::Black, l, p, r.left()), r.piece(),
                        r.right().blacken(A));
        }
        return *this;
    }

    static RedBlackTree ins(
        NodeArena& A, const RedBlackTree& node, size_t at, const Piece& p, size_t total_offset) {
        if (!node) return make(A, Color::Red, {}, p, {});

        size_t split = total_offset + node.left_length() + node.piece().length;
        if (at < split) {
            RedBlackTree new_left = ins(A, node.left(), at, p, total_offset);
            return make(A, node.color(), new_left, node.piece(), node.right()).balance(A);
        } else {
            RedBlackTree new_right = ins(A, node.right(), at, p, split);
            return make(A, node.color(), node.left(), node.piece(), new_right).balance(A);
        }
    }
};

}  // namespace editor
