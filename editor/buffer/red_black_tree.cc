#include "base/check.h"
#include "editor/buffer/red_black_tree.h"

namespace editor {

namespace {

// Null nodes are black.
inline bool is_black(const RedBlackTree& t) { return !t || t.color() == Color::Black; }
inline bool is_red(const RedBlackTree& t) { return t && t.color() == Color::Red; }
inline bool doubled_left(const RedBlackTree& t) { return is_red(t) && is_red(t.left()); }
inline bool doubled_right(const RedBlackTree& t) { return is_red(t) && is_red(t.right()); }

inline RedBlackTree paint(const RedBlackTree& node, Color c) {
    DCHECK(node);
    return {c, node.left(), node.piece(), node.right()};
}

RedBlackTree balance(Color c, const RedBlackTree& lft, const Piece& p, const RedBlackTree& rgt);
RedBlackTree balance(const RedBlackTree& node);
RedBlackTree fuse(const RedBlackTree& left, const RedBlackTree& right);
RedBlackTree balance_left(const RedBlackTree& left);
RedBlackTree balance_right(const RedBlackTree& right);
RedBlackTree remove_left(const RedBlackTree& root, size_t at, size_t total);
RedBlackTree remove_right(const RedBlackTree& root, size_t at, size_t total);
RedBlackTree rem(const RedBlackTree& root, size_t at, size_t total);
RedBlackTree internal_insert(const RedBlackTree& node,
                             const Piece& x,
                             size_t at,
                             size_t total_offset);

}  // namespace

RedBlackTree::RedBlackTree(Color c,
                           const RedBlackTree& lft,
                           const Piece& p,
                           const RedBlackTree& rgt) {
    NodeData d = {
        .piece = p,
        .left_length = lft.length(),
        .left_lf_count = lft.line_feed_count(),
        .subtree_length = lft.length() + p.length + rgt.length(),
        .subtree_lf_count = lft.line_feed_count() + p.lf_count + rgt.line_feed_count(),
    };
    node_ = std::make_shared<Node>(c, lft.node_, d, rgt.node_);
}

RedBlackTree RedBlackTree::insert(size_t at, const Piece& p) const {
    RedBlackTree t = internal_insert(*this, p, at, 0);
    return {Color::Black, t.left(), t.piece(), t.right()};
}

RedBlackTree RedBlackTree::remove(size_t at) const {
    auto t = rem(*this, at, 0);
    if (!t) return {};
    return {Color::Black, t.left(), t.piece(), t.right()};
}

namespace {
// Borrowed from https://github.com/dotnwat/persistent-rbtree/blob/master/tree.h:checkConsistency.
int check_black_node_invariant(const RedBlackTree& node) {
    if (!node) return 1;
    if (is_red(node) && (is_red(node.left()) || is_red(node.right()))) return 0;

    auto l = check_black_node_invariant(node.left());
    auto r = check_black_node_invariant(node.right());

    if (l != 0 && r != 0 && l != r) return 0;
    if (l != 0 && r != 0) return is_red(node) ? l : l + 1;
    return 0;
}
}  // namespace

bool RedBlackTree::check_invariants() const {
    // 1. Every node is either red or black.
    // 2. The root is black.
    // 3. Every leaf (NIL) is black.
    // 4. If a node is red, then both its children are black.
    // 5. For each node, all simple paths from the node to descendant leaves contain the same
    // number of black nodes.

    // The internal nodes in this RB tree can be totally black so we will not count them directly,
    // we'll just track odd nodes as either red or black. Measure the number of black nodes we need
    // to validate.
    const auto& root = *this;
    // TODO: Do we need this line?
    // if (!root || (!left() && !right())) return true;
    // TODO: Can we incorporate this check in `check_black_node_invariant`?
    if (!is_black(root)) return false;
    return check_black_node_invariant(root) != 0;
}

namespace {

RedBlackTree balance(Color c, const RedBlackTree& lft, const Piece& p, const RedBlackTree& rgt) {
    if (c == Color::Black && doubled_left(lft)) {
        return {Color::Red,
                paint(lft.left(), Color::Black),
                lft.piece(),
                {Color::Black, lft.right(), p, rgt}};
    }
    if (c == Color::Black && doubled_right(lft)) {
        return {Color::Red,
                {Color::Black, lft.left(), lft.piece(), lft.right().left()},
                lft.right().piece(),
                {Color::Black, lft.right().right(), p, rgt}};
    }
    if (c == Color::Black && doubled_left(rgt)) {
        return {Color::Red,
                {Color::Black, lft, p, rgt.left().left()},
                rgt.left().piece(),
                {Color::Black, rgt.left().right(), rgt.piece(), rgt.right()}};
    }
    if (c == Color::Black && doubled_right(rgt)) {
        return {Color::Red,
                {Color::Black, lft, p, rgt.left()},
                rgt.piece(),
                paint(rgt.right(), Color::Black)};
    }
    return {c, lft, p, rgt};
}

RedBlackTree balance(const RedBlackTree& node) {
    // Two red children.
    if (is_red(node.left()) && is_red(node.right())) {
        auto l = paint(node.left(), Color::Black);
        auto r = paint(node.right(), Color::Black);
        return {Color::Red, l, node.piece(), r};
    }

    DCHECK_EQ(node.color(), Color::Black);
    return balance(node.color(), node.left(), node.piece(), node.right());
}

RedBlackTree fuse(const RedBlackTree& left, const RedBlackTree& right) {
    // match: (left, right)
    // case: (None, r)
    if (!left) return right;
    if (!right) return left;
    // match: (left.color, right.color)
    // case: (B, R)
    if (is_black(left) && is_red(right)) {
        return {Color::Red, fuse(left, right.left()), right.piece(), right.right()};
    }
    // case: (R, B)
    if (is_red(left) && is_black(right)) {
        return {Color::Red, left.left(), left.piece(), fuse(left.right(), right)};
    }
    // case: (R, R)
    if (is_red(left) && is_red(right)) {
        auto fused = fuse(left.right(), right.left());
        if (is_red(fused)) {
            RedBlackTree new_left = {Color::Red, left.left(), left.piece(), fused.left()};
            RedBlackTree new_right = {Color::Red, fused.right(), right.piece(), right.right()};
            return {Color::Red, new_left, fused.piece(), new_right};
        }
        RedBlackTree new_right = {Color::Red, fused, right.piece(), right.right()};
        return {Color::Red, left.left(), left.piece(), new_right};
    }
    // case: (B, B)
    DCHECK(is_black(left) && is_black(right));
    auto fused = fuse(left.right(), right.left());
    if (is_red(fused)) {
        RedBlackTree new_left = {Color::Black, left.left(), left.piece(), fused.left()};
        RedBlackTree new_right = {Color::Black, fused.right(), right.piece(), right.right()};
        return {Color::Red, new_left, fused.piece(), new_right};
    }
    RedBlackTree new_right = {Color::Black, fused, right.piece(), right.right()};
    RedBlackTree new_node = {Color::Red, left.left(), left.piece(), new_right};
    return balance_left(new_node);
}

RedBlackTree balance_left(const RedBlackTree& left) {
    // match: (color_l, color_r, color_r_l)
    // case: (Some(R), ..)
    if (left.left() && left.left().color() == Color::Red) {
        return {Color::Red, paint(left.left(), Color::Black), left.piece(), left.right()};
    }
    // case: (_, Some(B), _)
    if (left.right() && left.right().color() == Color::Black) {
        RedBlackTree new_left = {Color::Black, left.left(), left.piece(),
                                 paint(left.right(), Color::Red)};
        return balance(new_left);
    }
    // case: (_, Some(R), Some(B))
    if (left.right() && left.right().color() == Color::Red && left.right().left() &&
        left.right().left().color() == Color::Black) {
        RedBlackTree unbalanced_new_right = {Color::Black, left.right().left().right(),
                                             left.right().piece(),
                                             paint(left.right().right(), Color::Red)};
        auto new_right = balance(unbalanced_new_right);
        RedBlackTree new_left = {Color::Black, left.left(), left.piece(),
                                 left.right().left().left()};
        return {Color::Red, new_left, left.right().left().piece(), new_right};
    }
    NOTREACHED();
}

RedBlackTree balance_right(const RedBlackTree& right) {
    // match: (color_l, color_l_r, color_r)
    // case: (.., Some(R))
    if (right.right() && right.right().color() == Color::Red) {
        return {Color::Red, right.left(), right.piece(), paint(right.right(), Color::Black)};
    }
    // case: (Some(B), ..)
    if (right.left() && right.left().color() == Color::Black) {
        RedBlackTree new_right = {Color::Black, paint(right.left(), Color::Red), right.piece(),
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
            right.left().piece(),
            right.left().right().left(),
        };
        auto new_left = balance(unbalanced_new_left);
        RedBlackTree new_right = {Color::Black, right.left().right().right(), right.piece(),
                                  right.right()};
        return {Color::Red, new_left, right.left().right().piece(), new_right};
    }
    NOTREACHED();
}

RedBlackTree remove_left(const RedBlackTree& root, size_t at, size_t total) {
    auto new_left = rem(root.left(), at, total);
    RedBlackTree new_node = {Color::Red, new_left, root.piece(), root.right()};
    // In this case, the root was a red node and must've had at least two children.
    if (root.left() && root.left().color() == Color::Black) {
        return balance_left(new_node);
    }
    return new_node;
}

RedBlackTree remove_right(const RedBlackTree& root, size_t at, size_t total) {
    auto new_right = rem(root.right(), at, total + root.left_length() + root.piece().length);
    RedBlackTree new_node = {Color::Red, root.left(), root.piece(), new_right};
    // In this case, the root was a red node and must've had at least two children.
    if (root.right() && root.right().color() == Color::Black) {
        return balance_right(new_node);
    }
    return new_node;
}

RedBlackTree rem(const RedBlackTree& root, size_t at, size_t total) {
    if (!root) return {};
    if (at < total + root.left_length()) return remove_left(root, at, total);
    if (at == total + root.left_length()) return fuse(root.left(), root.right());
    return remove_right(root, at, total);
}

RedBlackTree internal_insert(const RedBlackTree& node,
                             const Piece& p,
                             size_t at,
                             size_t total_offset) {
    if (!node) {
        return {Color::Red, {}, p, {}};
    }

    if (at < total_offset + node.left_length() + node.piece().length) {
        return balance(node.color(), internal_insert(node.left(), p, at, total_offset),
                       node.piece(), node.right());
    }
    auto rgt = internal_insert(node.right(), p, at,
                               total_offset + node.left_length() + node.piece().length);
    return balance(node.color(), node.left(), node.piece(), rgt);
}

}  // namespace

}  // namespace editor
