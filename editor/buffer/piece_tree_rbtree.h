#pragma once

#include <memory>

namespace editor {

struct BufferCursor {
    size_t line{};    // Relative line in the current buffer.
    size_t column{};  // Column into the current line.

    bool operator==(const BufferCursor&) const = default;
};

enum class BufferType { Original, Mod };

struct Piece {
    BufferType buffer_type = BufferType::Original;
    BufferCursor first{};
    BufferCursor last{};
    size_t length{};
    size_t newline_count{};
};

struct NodeData {
    Piece piece;
    size_t left_subtree_length = 0;
    size_t left_subtree_lf_count = 0;
};

enum class Color { Red, Black, DoubleBlack };

class RedBlackTree {
    struct Node;
    using NodePtr = std::shared_ptr<const Node>;

    struct Node {
        Node(Color c, const NodePtr& lft, const NodeData& data, const NodePtr& rgt);

        Color color;
        NodePtr left;
        NodeData data;
        NodePtr right;
    };

public:
    RedBlackTree() = default;

    // Queries.
    constexpr const Node* root_ptr() const { return root_node_.get(); }
    constexpr bool empty() const { return !root_node_; }
    const NodeData& data() const;
    RedBlackTree left() const;
    RedBlackTree right() const;
    Color root_color() const;
    size_t length() const;
    size_t lf_count() const;

    // Helpers.
    bool operator==(const RedBlackTree&) const = default;

    // Mutators.
    RedBlackTree insert(const NodeData& x, size_t at) const;
    RedBlackTree remove(size_t at) const;

private:
    RedBlackTree(Color c, const RedBlackTree& lft, const NodeData& val, const RedBlackTree& rgt);
    RedBlackTree(const NodePtr& node);

    static RedBlackTree fuse(const RedBlackTree& left, const RedBlackTree& right);
    static RedBlackTree balance(const RedBlackTree& node);
    static RedBlackTree balance_left(const RedBlackTree& left);
    static RedBlackTree balance_right(const RedBlackTree& right);
    static RedBlackTree remove_left(const RedBlackTree& root, size_t at, size_t total);
    static RedBlackTree remove_right(const RedBlackTree& root, size_t at, size_t total);
    static RedBlackTree rem(const RedBlackTree& root, size_t at, size_t total);

    // Insertion.
    RedBlackTree internal_insert(const NodeData& x, size_t at, size_t total_offset) const;
    static RedBlackTree balance(Color c,
                                const RedBlackTree& lft,
                                const NodeData& x,
                                const RedBlackTree& rgt);
    bool doubled_left() const;
    bool doubled_right() const;

    // General.
    RedBlackTree paint(Color c) const;

    NodePtr root_node_;
};

}  // namespace editor
