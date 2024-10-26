#pragma once

#include <memory>

namespace PieceTree {

struct BufferCursor {
    // Relative line in the current buffer.
    size_t line = 0;
    // Column into the current line.
    size_t column = 0;

    bool operator==(const BufferCursor&) const = default;
};

struct Piece {
    size_t index = 0;  // Index into a buffer in PieceTree.  This could be an immutable
                       // buffer or the mutable buffer.
    BufferCursor first = {};
    BufferCursor last = {};
    size_t length = 0;
    size_t newline_count = 0;
};

struct NodeData {
    PieceTree::Piece piece;

    size_t left_subtree_length = 0;
    size_t left_subtree_lf_count = 0;
};

class RedBlackTree;

NodeData attribute(const NodeData& data, const RedBlackTree& left);

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
    struct ColorTree;

    explicit RedBlackTree() = default;

    // Queries.
    const Node* root_ptr() const;
    bool empty() const;
    const NodeData& root() const;
    RedBlackTree left() const;
    RedBlackTree right() const;
    Color root_color() const;

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
    RedBlackTree ins(const NodeData& x, size_t at, size_t total_offset) const;
    static RedBlackTree balance(Color c,
                                const RedBlackTree& lft,
                                const NodeData& x,
                                const RedBlackTree& rgt);
    bool doubled_left() const;
    bool doubled_right() const;

    // General.
    RedBlackTree paint(Color c) const;

    NodePtr root_node;
};

// Global queries.
size_t tree_length(const RedBlackTree& root);
size_t tree_lf_count(const RedBlackTree& root);

}  // namespace PieceTree
