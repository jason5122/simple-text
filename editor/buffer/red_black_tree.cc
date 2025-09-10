#include "base/check.h"
#include "editor/buffer/red_black_tree.h"

namespace editor {

namespace {

NodeData attribute(const NodeData& data, const RedBlackTree& left, const RedBlackTree& right) {
    NodeData d = data;
    d.left_subtree_length = left.length();
    d.left_subtree_lf_count = left.line_feed_count();
    d.subtree_length = d.left_subtree_length + d.piece.length + right.length();
    d.subtree_lf_count = d.left_subtree_lf_count + d.piece.newline_count + right.line_feed_count();
    return d;
}

bool doubled_left(const RedBlackTree& node) {
    return node && node.color() == Color::Red && node.left() && node.left().color() == Color::Red;
}

bool doubled_right(const RedBlackTree& node) {
    return node && node.color() == Color::Red && node.right() &&
           node.right().color() == Color::Red;
}

RedBlackTree paint(const RedBlackTree& node, Color c) {
    DCHECK(node);
    return {c, node.left(), node.data(), node.right()};
}

RedBlackTree balance(Color c, const RedBlackTree& lft, const NodeData& x, const RedBlackTree& rgt);
RedBlackTree balance(const RedBlackTree& node);
RedBlackTree fuse(const RedBlackTree& left, const RedBlackTree& right);
RedBlackTree balance_left(const RedBlackTree& left);
RedBlackTree balance_right(const RedBlackTree& right);
RedBlackTree remove_left(const RedBlackTree& root, size_t at, size_t total);
RedBlackTree remove_right(const RedBlackTree& root, size_t at, size_t total);
RedBlackTree rem(const RedBlackTree& root, size_t at, size_t total);
RedBlackTree internal_insert(const RedBlackTree& node,
                             const NodeData& x,
                             size_t at,
                             size_t total_offset);

}  // namespace

RedBlackTree::Node::Node(Color c, const NodePtr& lft, const NodeData& data, const NodePtr& rgt)
    : color(c), left(lft), data(data), right(rgt) {}

RedBlackTree RedBlackTree::insert(const NodeData& x, size_t at) const {
    RedBlackTree t = internal_insert(*this, x, at, 0);
    return {Color::Black, t.left(), t.data(), t.right()};
}

RedBlackTree::RedBlackTree(Color c,
                           const RedBlackTree& lft,
                           const NodeData& val,
                           const RedBlackTree& rgt)
    : node_(std::make_shared<Node>(c, lft.node_, attribute(val, lft, rgt), rgt.node_)) {}

RedBlackTree::RedBlackTree(const NodePtr& node) : node_(node) {}

RedBlackTree RedBlackTree::remove(size_t at) const {
    auto t = rem(*this, at, 0);
    if (!t) return {};
    return {Color::Black, t.left(), t.data(), t.right()};
}

namespace {

RedBlackTree balance(Color c,
                     const RedBlackTree& lft,
                     const NodeData& x,
                     const RedBlackTree& rgt) {
    if (c == Color::Black && doubled_left(lft))
        return {
            Color::Red,
            paint(lft.left(), Color::Black),
            lft.data(),
            {Color::Black, lft.right(), x, rgt},
        };
    else if (c == Color::Black && doubled_right(lft))
        return {
            Color::Red,
            {Color::Black, lft.left(), lft.data(), lft.right().left()},
            lft.right().data(),
            {Color::Black, lft.right().right(), x, rgt},
        };
    else if (c == Color::Black && doubled_left(rgt))
        return {
            Color::Red,
            {Color::Black, lft, x, rgt.left().left()},
            rgt.left().data(),
            {Color::Black, rgt.left().right(), rgt.data(), rgt.right()},
        };
    else if (c == Color::Black && doubled_right(rgt))
        return {
            Color::Red,
            {Color::Black, lft, x, rgt.left()},
            rgt.data(),
            paint(rgt.right(), Color::Black),
        };
    return {c, lft, x, rgt};
}

RedBlackTree balance(const RedBlackTree& node) {
    // Two red children.
    if (node.left() && node.left().color() == Color::Red && node.right() &&
        node.right().color() == Color::Red) {
        auto l = paint(node.left(), Color::Black);
        auto r = paint(node.right(), Color::Black);
        return {Color::Red, l, node.data(), r};
    }

    DCHECK_EQ(node.color(), Color::Black);
    return balance(node.color(), node.left(), node.data(), node.right());
}

RedBlackTree fuse(const RedBlackTree& left, const RedBlackTree& right) {
    // match: (left, right)
    // case: (None, r)
    if (!left) return right;
    if (!right) return left;
    // match: (left.color, right.color)
    // case: (B, R)
    if (left.color() == Color::Black && right.color() == Color::Red) {
        return {Color::Red, fuse(left, right.left()), right.data(), right.right()};
    }
    // case: (R, B)
    if (left.color() == Color::Red && right.color() == Color::Black) {
        return {Color::Red, left.left(), left.data(), fuse(left.right(), right)};
    }
    // case: (R, R)
    if (left.color() == Color::Red && right.color() == Color::Red) {
        auto fused = fuse(left.right(), right.left());
        if (fused && fused.color() == Color::Red) {
            RedBlackTree new_left = {Color::Red, left.left(), left.data(), fused.left()};
            RedBlackTree new_right = {Color::Red, fused.right(), right.data(), right.right()};
            return {Color::Red, new_left, fused.data(), new_right};
        }
        RedBlackTree new_right = {Color::Red, fused, right.data(), right.right()};
        return {Color::Red, left.left(), left.data(), new_right};
    }
    // case: (B, B)
    DCHECK(left.color() == right.color() && left.color() == Color::Black);
    auto fused = fuse(left.right(), right.left());
    if (fused && fused.color() == Color::Red) {
        RedBlackTree new_left = {Color::Black, left.left(), left.data(), fused.left()};
        RedBlackTree new_right = {Color::Black, fused.right(), right.data(), right.right()};
        return {Color::Red, new_left, fused.data(), new_right};
    }
    RedBlackTree new_right = {Color::Black, fused, right.data(), right.right()};
    RedBlackTree new_node = {Color::Red, left.left(), left.data(), new_right};
    return balance_left(new_node);
}

RedBlackTree balance_left(const RedBlackTree& left) {
    // match: (color_l, color_r, color_r_l)
    // case: (Some(R), ..)
    if (left.left() && left.left().color() == Color::Red) {
        return {Color::Red, paint(left.left(), Color::Black), left.data(), left.right()};
    }
    // case: (_, Some(B), _)
    if (left.right() && left.right().color() == Color::Black) {
        RedBlackTree new_left = {Color::Black, left.left(), left.data(),
                                 paint(left.right(), Color::Red)};
        return balance(new_left);
    }
    // case: (_, Some(R), Some(B))
    if (left.right() && left.right().color() == Color::Red && left.right().left() &&
        left.right().left().color() == Color::Black) {
        RedBlackTree unbalanced_new_right = {Color::Black, left.right().left().right(),
                                             left.right().data(),
                                             paint(left.right().right(), Color::Red)};
        auto new_right = balance(unbalanced_new_right);
        RedBlackTree new_left = {Color::Black, left.left(), left.data(),
                                 left.right().left().left()};
        return {Color::Red, new_left, left.right().left().data(), new_right};
    }
    NOTREACHED();
}

RedBlackTree balance_right(const RedBlackTree& right) {
    // match: (color_l, color_l_r, color_r)
    // case: (.., Some(R))
    if (right.right() && right.right().color() == Color::Red) {
        return {Color::Red, right.left(), right.data(), paint(right.right(), Color::Black)};
    }
    // case: (Some(B), ..)
    if (right.left() && right.left().color() == Color::Black) {
        RedBlackTree new_right = {Color::Black, paint(right.left(), Color::Red), right.data(),
                                  right.right()};
        return balance(new_right);
    }
    // case: (Some(R), Some(B), _)
    if (right.left() && right.left().color() == Color::Red && right.left().right() &&
        right.left().right().color() == Color::Black) {
        RedBlackTree unbalanced_new_left = {
            Color::Black,
            // Note: Because 'left' is red, it must have a left child.
            paint(right.left().left(), Color::Red),
            right.left().data(),
            right.left().right().left(),
        };
        auto new_left = balance(unbalanced_new_left);
        RedBlackTree new_right = {Color::Black, right.left().right().right(), right.data(),
                                  right.right()};
        return {Color::Red, new_left, right.left().right().data(), new_right};
    }
    NOTREACHED();
}

RedBlackTree remove_left(const RedBlackTree& root, size_t at, size_t total) {
    auto new_left = rem(root.left(), at, total);
    RedBlackTree new_node = {Color::Red, new_left, root.data(), root.right()};
    // In this case, the root was a red node and must've had at least two children.
    if (root.left() && root.left().color() == Color::Black) {
        return balance_left(new_node);
    }
    return new_node;
}

RedBlackTree remove_right(const RedBlackTree& root, size_t at, size_t total) {
    const NodeData& y = root.data();
    auto new_right = rem(root.right(), at, total + y.left_subtree_length + y.piece.length);
    RedBlackTree new_node = {Color::Red, root.left(), root.data(), new_right};
    // In this case, the root was a red node and must've had at least two children.
    if (root.right() && root.right().color() == Color::Black) {
        return balance_right(new_node);
    }
    return new_node;
}

RedBlackTree rem(const RedBlackTree& root, size_t at, size_t total) {
    if (!root) return {};
    const NodeData& y = root.data();
    if (at < total + y.left_subtree_length) return remove_left(root, at, total);
    if (at == total + y.left_subtree_length) return fuse(root.left(), root.right());
    return remove_right(root, at, total);
}

RedBlackTree internal_insert(const RedBlackTree& node,
                             const NodeData& x,
                             size_t at,
                             size_t total_offset) {
    if (!node) {
        return {Color::Red, {}, x, {}};
    }

    const NodeData& y = node.data();
    if (at < total_offset + y.left_subtree_length + y.piece.length) {
        return balance(node.color(), internal_insert(node.left(), x, at, total_offset), y,
                       node.right());
    }
    auto rgt = internal_insert(node.right(), x, at,
                               total_offset + y.left_subtree_length + y.piece.length);
    return balance(node.color(), node.left(), y, rgt);
}

}  // namespace

}  // namespace editor
