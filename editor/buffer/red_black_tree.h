#pragma once

#include "base/check.h"
#include <memory>

namespace editor {

struct BufferCursor {
    size_t line{};    // Relative line in the current buffer.
    size_t column{};  // Column into the current line.

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

struct NodeData {
    Piece piece{};
    size_t left_length{};
    size_t left_lf_count{};
    size_t subtree_length{};
    size_t subtree_lf_count{};
};

// TODO: Should we hide `Color`/`color()` as an implementation detail? We already expose
// `is_red()`/`is_black()` checks.
enum class Color { Red, Black };

class RedBlackTree {
    struct Node;
    using NodePtr = std::shared_ptr<const Node>;

    struct Node {
        Color color;
        NodePtr left;
        NodeData data;
        NodePtr right;
    };

public:
    RedBlackTree() = default;
    RedBlackTree(Color c, const RedBlackTree& left, const Piece& p, const RedBlackTree& right);

    // Queries.
    explicit operator bool() const { return static_cast<bool>(node_); }
    size_t length() const { return !node_ ? 0 : node_->data.subtree_length; }
    size_t line_feed_count() const { return !node_ ? 0 : node_->data.subtree_lf_count; }
    size_t left_length() const { return !node_ ? 0 : node_->data.left_length; }
    size_t left_line_feed_count() const { return !node_ ? 0 : node_->data.left_lf_count; }
    // clang-format off
    const Piece& piece() const { DCHECK(node_); return node_->data.piece; }
    // TODO: See above comment under `Color`.
    Color color() const { DCHECK(node_); return node_->color; }
    RedBlackTree left() const { DCHECK(node_); return {node_->left}; }
    RedBlackTree right() const { DCHECK(node_); return {node_->right}; }
    // clang-format on

    // Mutators.
    RedBlackTree insert(size_t at, const Piece& p) const;
    RedBlackTree remove(size_t at) const;

    // Helpers.
    bool operator==(const RedBlackTree&) const = default;

    // Debug use.
    bool satisfies_red_black_invariants() const;

    // TODO: Organize and test these.
    // Null nodes are black.
    bool is_black() const { return !node_ || node_->color == Color::Black; }
    bool is_red() const { return node_ && node_->color == Color::Red; }
    bool double_red_left() const { return is_red() && left().is_red(); }
    bool double_red_right() const { return is_red() && right().is_red(); }
    RedBlackTree blacken() const { return {Color::Black, left(), piece(), right()}; }
    RedBlackTree redden() const { return {Color::Red, left(), piece(), right()}; }
    RedBlackTree balance() const;
    // TODO: These are associated with deletion. Consider renaming to `fixup_delete_{left,right}`.
    RedBlackTree balance_left() const;
    RedBlackTree balance_right() const;
    RedBlackTree remove_left(size_t at, size_t total) const;
    RedBlackTree remove_right(size_t at, size_t total) const;

private:
    RedBlackTree(const NodePtr& node) : node_(node) {}

    NodePtr node_;
};

}  // namespace editor
