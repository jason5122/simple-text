#include "fredbuf.h"

#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/numeric/saturation_arithmetic.h"
#include "util/scope_guard.h"
#include "util/std_print.h"

namespace PieceTree {

RedBlackTree::Node::Node(Color c, const NodePtr& lft, const NodeData& data, const NodePtr& rgt)
    : color(c), left(lft), data(data), right(rgt) {}

const RedBlackTree::Node* RedBlackTree::root_ptr() const {
    return root_node.get();
}

bool RedBlackTree::empty() const {
    return !root_node;
}

const NodeData& RedBlackTree::root() const {
    assert(!empty());
    return root_node->data;
}

RedBlackTree RedBlackTree::left() const {
    assert(!empty());
    return RedBlackTree(root_node->left);
}

RedBlackTree RedBlackTree::right() const {
    assert(!empty());
    return RedBlackTree(root_node->right);
}

Color RedBlackTree::root_color() const {
    assert(!empty());
    return root_node->color;
}

RedBlackTree RedBlackTree::insert(const NodeData& x, size_t at) const {
    RedBlackTree t = ins(x, at, 0);
    return RedBlackTree(Color::Black, t.left(), t.root(), t.right());
}

RedBlackTree::RedBlackTree(Color c,
                           const RedBlackTree& lft,
                           const NodeData& val,
                           const RedBlackTree& rgt)
    : root_node(std::make_shared<Node>(c, lft.root_node, attribute(val, lft), rgt.root_node)) {}

RedBlackTree::RedBlackTree(const NodePtr& node) : root_node(node) {}

RedBlackTree RedBlackTree::ins(const NodeData& x, size_t at, size_t total_offset) const {
    if (empty()) {
        return RedBlackTree(Color::Red, RedBlackTree(), x, RedBlackTree());
    }

    const NodeData& y = root();
    if (at < total_offset + y.left_subtree_length + y.piece.length) {
        return balance(root_color(), left().ins(x, at, total_offset), y, right());
    }
    auto rgt = right().ins(x, at, total_offset + y.left_subtree_length + y.piece.length);
    return balance(root_color(), left(), y, rgt);
}

RedBlackTree RedBlackTree::balance(Color c,
                                   const RedBlackTree& lft,
                                   const NodeData& x,
                                   const RedBlackTree& rgt) {
    if (c == Color::Black && lft.doubled_left())
        return RedBlackTree(Color::Red, lft.left().paint(Color::Black), lft.root(),
                            RedBlackTree(Color::Black, lft.right(), x, rgt));
    else if (c == Color::Black && lft.doubled_right())
        return RedBlackTree(
            Color::Red, RedBlackTree(Color::Black, lft.left(), lft.root(), lft.right().left()),
            lft.right().root(), RedBlackTree(Color::Black, lft.right().right(), x, rgt));
    else if (c == Color::Black && rgt.doubled_left())
        return RedBlackTree(
            Color::Red, RedBlackTree(Color::Black, lft, x, rgt.left().left()), rgt.left().root(),
            RedBlackTree(Color::Black, rgt.left().right(), rgt.root(), rgt.right()));
    else if (c == Color::Black && rgt.doubled_right())
        return RedBlackTree(Color::Red, RedBlackTree(Color::Black, lft, x, rgt.left()), rgt.root(),
                            rgt.right().paint(Color::Black));
    return RedBlackTree(c, lft, x, rgt);
}

bool RedBlackTree::doubled_left() const {
    return !empty() && root_color() == Color::Red && !left().empty() &&
           left().root_color() == Color::Red;
}

bool RedBlackTree::doubled_right() const {
    return !empty() && root_color() == Color::Red && !right().empty() &&
           right().root_color() == Color::Red;
}

RedBlackTree RedBlackTree::paint(Color c) const {
    assert(!empty());
    return RedBlackTree(c, left(), root(), right());
}

size_t tree_length(const RedBlackTree& root) {
    if (root.empty()) return {};
    return root.root().left_subtree_length + root.root().piece.length + tree_length(root.right());
}

size_t tree_lf_count(const RedBlackTree& root) {
    if (root.empty()) return {};
    return root.root().left_subtree_lf_count + root.root().piece.newline_count +
           tree_lf_count(root.right());
}

NodeData attribute(const NodeData& data, const RedBlackTree& left) {
    NodeData new_data = data;
    new_data.left_subtree_length = tree_length(left);
    new_data.left_subtree_lf_count = tree_lf_count(left);
    return new_data;
}

struct RedBlackTree::ColorTree {
    const Color color;
    const RedBlackTree tree;

    static ColorTree double_black() {
        return ColorTree();
    }

    explicit ColorTree(RedBlackTree const& tree)
        : color(tree.empty() ? Color::Black : tree.root_color()), tree(tree) {}

    explicit ColorTree(Color c,
                       const RedBlackTree& lft,
                       const NodeData& x,
                       const RedBlackTree& rgt)
        : color(c), tree(c, lft, x, rgt) {}

private:
    ColorTree() : color(Color::DoubleBlack) {}
};

struct WalkResult {
    RedBlackTree tree;
    size_t accumulated_offset;
};

WalkResult pred(const RedBlackTree& root, size_t start_offset) {
    RedBlackTree t = root.left();
    while (!t.right().empty()) {
        start_offset = start_offset + t.root().left_subtree_length + t.root().piece.length;
        t = t.right();
    }
    // Add the final offset from the last right node.
    start_offset = start_offset + t.root().left_subtree_length;
    return {.tree = t, .accumulated_offset = start_offset};
}

RedBlackTree RedBlackTree::remove(size_t at) const {
    auto t = rem(*this, at, 0);
    if (t.empty()) return RedBlackTree();
    return RedBlackTree(Color::Black, t.left(), t.root(), t.right());
}

RedBlackTree RedBlackTree::fuse(const RedBlackTree& left, const RedBlackTree& right) {
    // match: (left, right)
    // case: (None, r)
    if (left.empty()) return right;
    if (right.empty()) return left;
    // match: (left.color, right.color)
    // case: (B, R)
    if (left.root_color() == Color::Black && right.root_color() == Color::Red) {
        return RedBlackTree(Color::Red, fuse(left, right.left()), right.root(), right.right());
    }
    // case: (R, B)
    if (left.root_color() == Color::Red && right.root_color() == Color::Black) {
        return RedBlackTree(Color::Red, left.left(), left.root(), fuse(left.right(), right));
    }
    // case: (R, R)
    if (left.root_color() == Color::Red && right.root_color() == Color::Red) {
        auto fused = fuse(left.right(), right.left());
        if (!fused.empty() && fused.root_color() == Color::Red) {
            auto new_left = RedBlackTree(Color::Red, left.left(), left.root(), fused.left());
            auto new_right = RedBlackTree(Color::Red, fused.right(), right.root(), right.right());
            return RedBlackTree(Color::Red, new_left, fused.root(), new_right);
        }
        auto new_right = RedBlackTree(Color::Red, fused, right.root(), right.right());
        return RedBlackTree(Color::Red, left.left(), left.root(), new_right);
    }
    // case: (B, B)
    assert(left.root_color() == right.root_color() && left.root_color() == Color::Black);
    auto fused = fuse(left.right(), right.left());
    if (!fused.empty() && fused.root_color() == Color::Red) {
        auto new_left = RedBlackTree(Color::Black, left.left(), left.root(), fused.left());
        auto new_right = RedBlackTree(Color::Black, fused.right(), right.root(), right.right());
        return RedBlackTree(Color::Red, new_left, fused.root(), new_right);
    }
    auto new_right = RedBlackTree(Color::Black, fused, right.root(), right.right());
    auto new_node = RedBlackTree(Color::Red, left.left(), left.root(), new_right);
    return balance_left(new_node);
}

RedBlackTree RedBlackTree::balance(const RedBlackTree& node) {
    // Two red children.
    if (!node.left().empty() && node.left().root_color() == Color::Red && !node.right().empty() &&
        node.right().root_color() == Color::Red) {
        auto l = node.left().paint(Color::Black);
        auto r = node.right().paint(Color::Black);
        return RedBlackTree(Color::Red, l, node.root(), r);
    }

    assert(node.root_color() == Color::Black);
    return balance(node.root_color(), node.left(), node.root(), node.right());
}

RedBlackTree RedBlackTree::balance_left(const RedBlackTree& left) {
    // match: (color_l, color_r, color_r_l)
    // case: (Some(R), ..)
    if (!left.left().empty() && left.left().root_color() == Color::Red) {
        return RedBlackTree(Color::Red, left.left().paint(Color::Black), left.root(),
                            left.right());
    }
    // case: (_, Some(B), _)
    if (!left.right().empty() && left.right().root_color() == Color::Black) {
        auto new_left =
            RedBlackTree(Color::Black, left.left(), left.root(), left.right().paint(Color::Red));
        return balance(new_left);
    }
    // case: (_, Some(R), Some(B))
    if (!left.right().empty() && left.right().root_color() == Color::Red &&
        !left.right().left().empty() && left.right().left().root_color() == Color::Black) {
        auto unbalanced_new_right =
            RedBlackTree(Color::Black, left.right().left().right(), left.right().root(),
                         left.right().right().paint(Color::Red));
        auto new_right = balance(unbalanced_new_right);
        auto new_left =
            RedBlackTree(Color::Black, left.left(), left.root(), left.right().left().left());
        return RedBlackTree(Color::Red, new_left, left.right().left().root(), new_right);
    }
    assert(!"impossible");
    return left;
}

RedBlackTree RedBlackTree::balance_right(const RedBlackTree& right) {
    // match: (color_l, color_l_r, color_r)
    // case: (.., Some(R))
    if (!right.right().empty() && right.right().root_color() == Color::Red) {
        return RedBlackTree(Color::Red, right.left(), right.root(),
                            right.right().paint(Color::Black));
    }
    // case: (Some(B), ..)
    if (!right.left().empty() && right.left().root_color() == Color::Black) {
        auto new_right = RedBlackTree(Color::Black, right.left().paint(Color::Red), right.root(),
                                      right.right());
        return balance(new_right);
    }
    // case: (Some(R), Some(B), _)
    if (!right.left().empty() && right.left().root_color() == Color::Red &&
        !right.left().right().empty() && right.left().right().root_color() == Color::Black) {
        auto unbalanced_new_left =
            RedBlackTree(Color::Black,
                         // Note: Because 'left' is red, it must have a left child.
                         right.left().left().paint(Color::Red), right.left().root(),
                         right.left().right().left());
        auto new_left = balance(unbalanced_new_left);
        auto new_right =
            RedBlackTree(Color::Black, right.left().right().right(), right.root(), right.right());
        return RedBlackTree(Color::Red, new_left, right.left().right().root(), new_right);
    }
    assert(!"impossible");
    return right;
}

RedBlackTree RedBlackTree::remove_left(const RedBlackTree& root, size_t at, size_t total) {
    auto new_left = rem(root.left(), at, total);
    auto new_node = RedBlackTree(Color::Red, new_left, root.root(), root.right());
    // In this case, the root was a red node and must've had at least two children.
    if (!root.left().empty() && root.left().root_color() == Color::Black)
        return balance_left(new_node);
    return new_node;
}

RedBlackTree RedBlackTree::remove_right(const RedBlackTree& root, size_t at, size_t total) {
    const NodeData& y = root.root();
    auto new_right = rem(root.right(), at, total + y.left_subtree_length + y.piece.length);
    auto new_node = RedBlackTree(Color::Red, root.left(), root.root(), new_right);
    // In this case, the root was a red node and must've had at least two children.
    if (!root.right().empty() && root.right().root_color() == Color::Black)
        return balance_right(new_node);
    return new_node;
}

RedBlackTree RedBlackTree::rem(const RedBlackTree& root, size_t at, size_t total) {
    if (root.empty()) return RedBlackTree();
    const NodeData& y = root.root();
    if (at < total + y.left_subtree_length) return remove_left(root, at, total);
    if (at == total + y.left_subtree_length) return fuse(root.left(), root.right());
    return remove_right(root, at, total);
}

#ifdef TEXTBUF_DEBUG
// Borrowed from https://github.com/dotnwat/persistent-rbtree/blob/master/tree.h:checkConsistency.
int check_black_node_invariant(const RedBlackTree& node) {
    if (node.empty()) return 1;
    if (node.root_color() == Color::Red &&
        ((!node.left().empty() && node.left().root_color() == Color::Red) ||
         (!node.right().empty() && node.right().root_color() == Color::Red))) {
        return 1;
    }
    auto l = check_black_node_invariant(node.left());
    auto r = check_black_node_invariant(node.right());

    if (l != 0 && r != 0 && l != r) return 0;

    if (l != 0 && r != 0) return node.root_color() == Color::Red ? l : l + 1;
    return 0;
}

void satisfies_rb_invariants(const RedBlackTree& root) {
    // 1. Every node is either red or black.
    // 2. All NIL nodes (figure 1) are considered black.
    // 3. A red node does not have a red child.
    // 4. Every path from a given node to any of its descendant NIL nodes goes through the same
    // number of black nodes.

    // The internal nodes in this RB tree can be totally black so we will not count them directly,
    // we'll just track odd nodes as either red or black. Measure the number of black nodes we need
    // to validate.
    if (root.empty() || (root.left().empty() && root.right().empty())) return;
    assert(check_black_node_invariant(root) != 0);
}
#endif  // TEXTBUF_DEBUG
}  // namespace PieceTree

namespace PieceTree {

const CharBuffer* BufferCollection::buffer_at(size_t index) const {
    if (index == kModBuffer) return &mod_buffer;
    return orig_buffers[index].get();
}

size_t BufferCollection::buffer_offset(size_t index, const BufferCursor& cursor) const {
    auto& starts = buffer_at(index)->line_starts;
    return starts[cursor.line] + cursor.column;
}

Tree::Tree() : buffers{} {
    build_tree();
}

Tree::Tree(Buffers&& buffers) : buffers{std::move(buffers)} {
    build_tree();
}

Tree::Tree(std::string_view txt) : buffers{} {
    build_tree();
    insert(0, txt);
}

void Tree::build_tree() {
    buffers.mod_buffer.line_starts.clear();
    buffers.mod_buffer.buffer.clear();
    // In order to maintain the invariant of other buffers, the mod_buffer needs a single
    // line-start of 0.
    buffers.mod_buffer.line_starts.push_back({});
    last_insert = {};

    const auto buf_count = buffers.orig_buffers.size();
    size_t offset = 0;
    for (size_t i = 0; i < buf_count; ++i) {
        const auto& buf = *buffers.orig_buffers[i];
        assert(!buf.line_starts.empty());
        // If this immutable buffer is empty, we can avoid creating a piece for it altogether.
        if (buf.buffer.empty()) continue;
        auto last_line = buf.line_starts.size() - 1;
        // Create a new node that spans this buffer and retains an index to it.
        // Insert the node into the balanced tree.
        Piece piece{
            .index = i,
            .first = {.line = 0, .column = 0},
            .last = {.line = last_line, .column = buf.buffer.size() - buf.line_starts[last_line]},
            .length = buf.buffer.size(),
            // Note: the number of newlines
            .newline_count = last_line};
        root = root.insert({piece}, offset);
        offset += piece.length;
    }

    compute_buffer_meta();
}

void Tree::internal_insert(size_t offset, std::string_view txt) {
    assert(!txt.empty());

    end_last_insert = offset + txt.size();
    ScopeGuard guard{[&] {
        compute_buffer_meta();
#ifdef TEXTBUF_DEBUG
        satisfies_rb_invariants(root);
#endif  // TEXTBUF_DEBUG
    }};
    if (root.empty()) {
        auto piece = build_piece(txt);
        root = root.insert({piece}, 0);
        return;
    }

    auto result = node_at(&buffers, root, offset);
    // If the offset is beyond the buffer, just select the last node.
    if (result.node == nullptr) {
        auto off = 0;
        if (total_content_length != 0) {
            off += total_content_length - 1;
        }
        result = node_at(&buffers, root, off);
    }

    // There are 3 cases:
    // 1. We are inserting at the beginning of an existing node.
    // 2. We are inserting at the end of an existing node.
    // 3. We are inserting in the middle of the node.
    auto [node, remainder, node_start_offset, line] = result;
    assert(node != nullptr);
    auto insert_pos = buffer_position(&buffers, node->piece, remainder);
    // Case #1.
    if (node_start_offset == offset) {
        // There's a bonus case here.  If our last insertion point was the same as this piece's
        // last and it inserted into the mod buffer, then we can simply 'extend' this piece by
        // the following process:
        // 1. Fetch the previous node (if we can) and compare.
        // 2. Build the new piece.
        // 3. Remove the old piece.
        // 4. Extend the old piece's length to the length of the newly created piece.
        // 5. Re-insert the new piece.
        if (offset != 0) {
            auto prev_node_result = node_at(&buffers, root, offset - 1);
            if (prev_node_result.node->piece.index == kModBuffer &&
                prev_node_result.node->piece.last == last_insert) {
                auto new_piece = build_piece(txt);
                combine_pieces(prev_node_result, new_piece);
                return;
            }
        }
        auto piece = build_piece(txt);
        root = root.insert({piece}, offset);
        return;
    }

    const bool inside_node = offset < node_start_offset + node->piece.length;

    // Case #2.
    if (!inside_node) {
        // There's a bonus case here.  If our last insertion point was the same as this piece's
        // last and it inserted into the mod buffer, then we can simply 'extend' this piece by
        // the following process:
        // 1. Build the new piece.
        // 2. Remove the old piece.
        // 3. Extend the old piece's length to the length of the newly created piece.
        // 4. Re-insert the new piece.
        if (node->piece.index == kModBuffer && node->piece.last == last_insert) {
            auto new_piece = build_piece(txt);
            combine_pieces(result, new_piece);
            return;
        }
        // Insert the new piece at the end.
        auto piece = build_piece(txt);
        root = root.insert({piece}, offset);
        return;
    }

    // Case #3.
    // The basic approach here is to split the existing node into two pieces
    // and insert the new piece in between them.
    auto new_len_right = buffers.buffer_offset(node->piece.index, node->piece.last) -
                         buffers.buffer_offset(node->piece.index, insert_pos);
    auto new_piece_right = node->piece;
    new_piece_right.first = insert_pos;
    new_piece_right.length = new_len_right;
    new_piece_right.newline_count =
        line_feed_count(&buffers, node->piece.index, insert_pos, node->piece.last);

    // Remove the original node tail.
    auto new_piece_left = trim_piece_right(&buffers, node->piece, insert_pos);

    auto new_piece = build_piece(txt);

    // Remove the original node.
    root = root.remove(node_start_offset);

    // Insert the left.
    root = root.insert({new_piece_left}, node_start_offset);

    // Insert the new mid.
    node_start_offset = node_start_offset + new_piece_left.length;
    root = root.insert({new_piece}, node_start_offset);

    // Insert remainder.
    node_start_offset = node_start_offset + new_piece.length;
    root = root.insert({new_piece_right}, node_start_offset);
}

void Tree::internal_remove(size_t offset, size_t count) {
    assert(count != 0 && !root.empty());
    ScopeGuard guard{[&] {
        compute_buffer_meta();
#ifdef TEXTBUF_DEBUG
        satisfies_rb_invariants(root);
#endif  // TEXTBUF_DEBUG
    }};
    auto first = node_at(&buffers, root, offset);
    auto last = node_at(&buffers, root, offset + count);
    auto first_node = first.node;
    auto last_node = last.node;

    auto start_split_pos = buffer_position(&buffers, first_node->piece, first.remainder);

    // Simple case: the range of characters we want to delete are
    // held directly within this node.  Remove the node, resize it
    // then add it back.
    if (first_node == last_node) {
        auto end_split_pos = buffer_position(&buffers, first_node->piece, last.remainder);
        // We're going to shrink the node starting from the beginning.
        if (first.start_offset == offset) {
            // Delete the entire node.
            if (count == first_node->piece.length) {
                root = root.remove(first.start_offset);
                return;
            }
            // Shrink the node.
            auto new_piece = trim_piece_left(&buffers, first_node->piece, end_split_pos);
            // Remove the old one and update.
            root = root.remove(first.start_offset).insert({new_piece}, first.start_offset);
            return;
        }

        // Trim the tail of this piece.
        if (first.start_offset + first_node->piece.length == offset + count) {
            auto new_piece = trim_piece_right(&buffers, first_node->piece, start_split_pos);
            // Remove the old one and update.
            root = root.remove(first.start_offset).insert({new_piece}, first.start_offset);
            return;
        }

        // print_tree(root, this);

        // The removed buffer is somewhere in the middle.  Trim it in both directions.
        auto [left, right] =
            shrink_piece(&buffers, first_node->piece, start_split_pos, end_split_pos);

        // std::println("left.first.line = {}, left.first.column = {}", left.first.line,
        //              left.first.column);
        // std::println("left.last.line = {}, left.last.column = {}", left.last.line,
        //              left.last.column);
        // std::println("right.first.line = {}, right.first.column = {}", right.first.line,
        //              right.first.column);
        // std::println("right.last.line = {}, right.last.column = {}", right.last.line,
        //              right.last.column);

        // if (count == 1) {
        //     std::println("GOTCHU");
        //     // print_piece(first_node->piece, this, 0);
        //     root = root.remove(first.start_offset);
        //     root = root.remove(first.start_offset);
        //     root = root.insert({right}, first.start_offset);
        //     root = root.insert({left}, first.start_offset);
        // } else {
        //     root = root.remove(first.start_offset)
        //                .insert({right}, first.start_offset)
        //                .insert({left}, first.start_offset);
        // }

        // TODO: How is this working??
        root = root.remove(first.start_offset);
        root = root.remove(first.start_offset);
        root = root.insert({right}, first.start_offset);
        root = root.insert({left}, first.start_offset);

        // root = root.remove(first.start_offset)
        //            // Note: We insert right first so that the 'left' will be inserted
        //            // to the right node's left.
        //            .insert({right}, first.start_offset)
        //            .insert({left}, first.start_offset);
        // print_tree(root, this);
        return;
    }

    // Traverse nodes and delete all nodes within the offset range. First we will build the
    // partial pieces for the nodes that will eventually make up this range.
    // There are four cases here:
    // 1. The entire first node is deleted as well as all of the last node.
    // 2. Part of the first node is deleted and all of the last node.
    // 3. Part of the first node is deleted and part of the last node.
    // 4. The entire first node is deleted and part of the last node.

    auto new_first = trim_piece_right(&buffers, first_node->piece, start_split_pos);
    if (last_node == nullptr) {
        remove_node_range(first, count);
    } else {
        // TODO: How is this working??
        root = root.remove(first.start_offset);
        root = root.remove(first.start_offset);

        auto end_split_pos = buffer_position(&buffers, last_node->piece, last.remainder);
        auto new_last = trim_piece_left(&buffers, last_node->piece, end_split_pos);
        remove_node_range(first, count);
        // There's an edge case here where we delete all the nodes up to 'last' but
        // last itself remains untouched.  The test of 'remainder' in 'last' can identify
        // this scenario to avoid inserting a duplicate of 'last'.
        if (last.remainder != 0) {
            if (new_last.length != 0) {
                root = root.insert({new_last}, first.start_offset);
            }
        }
    }

    if (new_first.length != 0) {
        root = root.insert({new_first}, first.start_offset);
    }
}

// Fetches the length of the piece starting from the first line to 'index' or to the end of
// the piece.
size_t Tree::accumulate_value(const BufferCollection* buffers, const Piece& piece, size_t index) {
    auto* buffer = buffers->buffer_at(piece.index);
    auto& line_starts = buffer->line_starts;
    // Extend it so we can capture the entire line content including newline.
    auto expected_start = piece.first.line + (index + 1);
    auto first = line_starts[piece.first.line] + piece.first.column;
    if (expected_start > piece.last.line) {
        auto last = line_starts[piece.last.line] + piece.last.column;
        return last - first;
    }
    auto last = line_starts[expected_start];
    return last - first;
}

// Fetches the length of the piece starting from the first line to 'index' or to the end of
// the piece.
size_t Tree::accumulate_value_no_lf(const BufferCollection* buffers,
                                    const Piece& piece,
                                    size_t index) {
    auto* buffer = buffers->buffer_at(piece.index);
    auto& line_starts = buffer->line_starts;
    // Extend it so we can capture the entire line content including newline.
    auto expected_start = piece.first.line + (index + 1);
    auto first = line_starts[piece.first.line] + piece.first.column;
    if (expected_start > piece.last.line) {
        auto last = line_starts[piece.last.line] + piece.last.column;
        if (last == first) return 0;
        if (buffer->buffer[last - 1] == '\n') return last - 1 - first;
        return last - first;
    }
    auto last = line_starts[expected_start];
    if (last == first) return 0;
    if (buffer->buffer[last - 1] == '\n') return last - 1 - first;
    return last - first;
}

template <Tree::Accumulator accumulate>
void Tree::line_start(size_t* offset,
                      const BufferCollection* buffers,
                      const PieceTree::RedBlackTree& node,
                      size_t line) {
    if (node.empty()) return;
    auto line_index = line;
    if (node.root().left_subtree_lf_count >= line_index) {
        line_start<accumulate>(offset, buffers, node.left(), line);
    }
    // The desired line is directly within the node.
    else if (node.root().left_subtree_lf_count + node.root().piece.newline_count >= line_index) {
        line_index -= node.root().left_subtree_lf_count;
        size_t len = node.root().left_subtree_length;
        if (line_index != 0) {
            len += (*accumulate)(buffers, node.root().piece, line_index - 1);
        }
        *offset += len;
    }
    // Assemble the LHS and RHS.
    else {
        // This case implies that 'left_subtree_lf_count' is strictly < line_index.
        // The content is somewhere in the middle.
        line_index -= node.root().left_subtree_lf_count + node.root().piece.newline_count;
        *offset += node.root().left_subtree_length + node.root().piece.length;
        line_start<accumulate>(offset, buffers, node.right(), line_index + 1);
    }
}

LineRange Tree::get_line_range(size_t line) const {
    LineRange range{};
    line_start<&Tree::accumulate_value>(&range.first, &buffers, root, line);
    line_start<&Tree::accumulate_value_no_lf>(&range.last, &buffers, root, line + 1);
    return range;
}

LineRange Tree::get_line_range_with_newline(size_t line) const {
    LineRange range{};
    line_start<&Tree::accumulate_value>(&range.first, &buffers, root, line);
    line_start<&Tree::accumulate_value>(&range.last, &buffers, root, line + 1);
    return range;
}

std::string Tree::str() const {
    std::string str;
    str.reserve(length());
    for (char ch : *this) {
        str.push_back(ch);
    }
    return str;
}

size_t Tree::length() const {
    return total_content_length;
}

bool Tree::empty() const {
    return total_content_length == 0;
}

size_t Tree::line_feed_count() const {
    return lf_count;
}

size_t Tree::line_count() const {
    return line_feed_count() + 1;
}

size_t Tree::line_at(size_t offset) const {
    if (empty()) return 0;
    auto result = node_at(&buffers, root, offset);
    return result.line;
}

std::pair<size_t, size_t> Tree::line_column_at(size_t offset) const {
    if (empty()) return {0, 0};
    auto result = node_at(&buffers, root, offset);
    size_t line = result.line;
    auto [first, last] = get_line_range(line);
    size_t col = offset - first;
    return {line, col};
}

size_t Tree::offset_at(size_t line, size_t column) const {
    auto [first, last] = get_line_range(line);
    size_t offset = std::min(first + column, last);
    return offset;
}

char Tree::at(size_t offset) const {
    return char_at(&buffers, root, offset);
}

char Tree::char_at(const BufferCollection* buffers, const RedBlackTree& node, size_t offset) {
    auto result = node_at(buffers, node, offset);
    if (result.node == nullptr) return '\0';
    auto* buffer = buffers->buffer_at(result.node->piece.index);
    auto buf_offset = buffers->buffer_offset(result.node->piece.index, result.node->piece.first);
    const char* p = buffer->buffer.data() + buf_offset + result.remainder;
    return *p;
}

void Tree::assemble_line(std::string* buf,
                         const PieceTree::RedBlackTree& node,
                         size_t line) const {
    if (node.empty()) return;

    size_t line_offset = 0;
    line_start<&Tree::accumulate_value>(&line_offset, &buffers, node, line);
    TreeWalker walker{this, line_offset};
    while (!walker.exhausted()) {
        char c = walker.next();
        if (c == '\n') break;
        buf->push_back(c);
    }
}

std::string Tree::get_line_content(size_t line) const {
    std::string buf;
    assemble_line(&buf, root, line);
    return buf;
}

size_t Tree::line_feed_count(const BufferCollection* buffers,
                             size_t index,
                             const BufferCursor& start,
                             const BufferCursor& end) {
    // If the end position is the beginning of a new line, then we can just return the difference
    // in lines.
    if (end.column == 0) return end.line - start.line;
    auto& starts = buffers->buffer_at(index)->line_starts;
    // It means, there is no LF after end.
    if (end.line == starts.size() - 1) return end.line - start.line;
    // Due to the check above, we know that there's at least one more line after 'end.line'.
    auto next_start_offset = starts[end.line + 1];
    auto end_offset = starts[end.line] + end.column;
    // There are more than 1 character after end, which means it can't be LF.
    if (next_start_offset > end_offset + 1) return end.line - start.line;
    // This must be the case.  next_start_offset is a line down, so it is not possible for
    // end_offset to be < it at this point.
    assert(end_offset + 1 == next_start_offset);
    return end.line - start.line;
}

namespace {
void populate_line_starts(LineStarts* starts, std::string_view buf) {
    starts->clear();
    starts->push_back(0);
    const auto len = buf.size();
    for (size_t i = 0; i < len; ++i) {
        char c = buf[i];
        if (c == '\n') {
            starts->push_back(i + 1);
        }
    }
}
}  // namespace [anon]

Piece Tree::build_piece(std::string_view txt) {
    auto start_offset = buffers.mod_buffer.buffer.size();
    populate_line_starts(&scratch_starts, txt);
    auto start = last_insert;
    // TODO: Handle CRLF (where the new buffer starts with LF and the end of our buffer ends with
    // CR). Offset the new starts relative to the existing buffer.
    for (auto& new_start : scratch_starts) {
        new_start += start_offset;
    }
    // Append new starts.
    // Note: we can drop the first start because the algorithm always adds an empty start.
    auto new_starts_end = scratch_starts.size();
    buffers.mod_buffer.line_starts.reserve(buffers.mod_buffer.line_starts.size() + new_starts_end);
    for (size_t i = 1; i < new_starts_end; ++i) {
        buffers.mod_buffer.line_starts.push_back(scratch_starts[i]);
    }
    auto old_size = buffers.mod_buffer.buffer.size();
    buffers.mod_buffer.buffer.resize(buffers.mod_buffer.buffer.size() + txt.size());
    auto insert_at = buffers.mod_buffer.buffer.data() + old_size;
    std::copy(txt.data(), txt.data() + txt.size(), insert_at);

    // Build the new piece for the inserted buffer.
    auto end_offset = buffers.mod_buffer.buffer.size();
    auto end_index = buffers.mod_buffer.line_starts.size() - 1;
    auto end_col = end_offset - buffers.mod_buffer.line_starts[end_index];
    BufferCursor end_pos = {.line = end_index, .column = end_col};
    Piece piece = {.index = kModBuffer,
                   .first = start,
                   .last = end_pos,
                   .length = end_offset - start_offset,
                   .newline_count = line_feed_count(&buffers, kModBuffer, start, end_pos)};
    // Update the last insertion.
    last_insert = end_pos;
    return piece;
}

NodePosition Tree::node_at(const BufferCollection* buffers, RedBlackTree node, size_t off) {
    size_t node_start_offset = 0;
    size_t newline_count = 0;
    while (!node.empty()) {
        if (node.root().left_subtree_length > off) {
            node = node.left();
        } else if (node.root().left_subtree_length + node.root().piece.length > off) {
            node_start_offset += node.root().left_subtree_length;
            newline_count += node.root().left_subtree_lf_count;
            // Now we find the line within this piece.
            auto remainder = off - node.root().left_subtree_length;
            auto pos = buffer_position(buffers, node.root().piece, remainder);
            // Note: since buffer_position will return us a newline relative to the buffer itself,
            // we need to retract it by the starting line of the piece to get the real difference.
            newline_count += pos.line - node.root().piece.first.line;
            return {.node = &node.root(),
                    .remainder = remainder,
                    .start_offset = node_start_offset,
                    .line = newline_count};
        } else {
            // If there are no more nodes to traverse to, return this final node.
            if (node.right().empty()) {
                auto offset_amount = node.root().left_subtree_length;
                node_start_offset += offset_amount;
                newline_count +=
                    node.root().left_subtree_lf_count + node.root().piece.newline_count;
                // Now we find the line within this piece.
                auto remainder = node.root().piece.length;
                return {.node = &node.root(),
                        .remainder = remainder,
                        .start_offset = node_start_offset,
                        .line = newline_count};
            }
            auto offset_amount = node.root().left_subtree_length + node.root().piece.length;
            off -= offset_amount;
            node_start_offset += offset_amount;
            newline_count += node.root().left_subtree_lf_count + node.root().piece.newline_count;
            node = node.right();
        }
    }
    return {};
}

BufferCursor Tree::buffer_position(const BufferCollection* buffers,
                                   const Piece& piece,
                                   size_t remainder) {
    auto& starts = buffers->buffer_at(piece.index)->line_starts;
    auto start_offset = starts[piece.first.line] + piece.first.column;
    auto offset = start_offset + remainder;

    // Binary search for 'offset' between start and ending offset.
    auto low = piece.first.line;
    auto high = piece.last.line;

    size_t mid = 0;
    size_t mid_start = 0;
    size_t mid_stop = 0;

    while (low <= high) {
        mid = low + ((high - low) / 2);
        mid_start = starts[mid];

        if (mid == high) break;
        mid_stop = starts[mid + 1];

        if (offset < mid_start) {
            high = mid - 1;
        } else if (offset >= mid_stop) {
            low = mid + 1;
        } else {
            break;
        }
    }

    return {.line = mid, .column = offset - mid_start};
}

Piece Tree::trim_piece_right(const BufferCollection* buffers,
                             const Piece& piece,
                             const BufferCursor& pos) {
    auto orig_end_offset = buffers->buffer_offset(piece.index, piece.last);

    auto new_end_offset = buffers->buffer_offset(piece.index, pos);
    auto new_lf_count = line_feed_count(buffers, piece.index, piece.first, pos);

    auto len_delta = orig_end_offset - new_end_offset;
    auto new_len = piece.length - len_delta;

    auto new_piece = piece;
    new_piece.last = pos;
    new_piece.newline_count = new_lf_count;
    new_piece.length = new_len;

    return new_piece;
}

Piece Tree::trim_piece_left(const BufferCollection* buffers,
                            const Piece& piece,
                            const BufferCursor& pos) {
    auto orig_start_offset = buffers->buffer_offset(piece.index, piece.first);

    auto new_start_offset = buffers->buffer_offset(piece.index, pos);
    auto new_lf_count = line_feed_count(buffers, piece.index, pos, piece.last);

    auto len_delta = new_start_offset - orig_start_offset;
    auto new_len = piece.length - len_delta;

    auto new_piece = piece;
    new_piece.first = pos;
    new_piece.newline_count = new_lf_count;
    new_piece.length = new_len;

    return new_piece;
}

Tree::ShrinkResult Tree::shrink_piece(const BufferCollection* buffers,
                                      const Piece& piece,
                                      const BufferCursor& first,
                                      const BufferCursor& last) {
    auto left = trim_piece_right(buffers, piece, first);
    auto right = trim_piece_left(buffers, piece, last);

    return {.left = left, .right = right};
}

void Tree::combine_pieces(NodePosition existing, Piece new_piece) {
    // This transformation is only valid under the following conditions.
    assert(existing.node->piece.index == kModBuffer);
    // This assumes that the piece was just built.
    assert(existing.node->piece.last == new_piece.first);
    auto old_piece = existing.node->piece;
    new_piece.first = old_piece.first;
    new_piece.newline_count = new_piece.newline_count + old_piece.newline_count;
    new_piece.length = new_piece.length + old_piece.length;
    root = root.remove(existing.start_offset).insert({new_piece}, existing.start_offset);
}

void Tree::remove_node_range(NodePosition first, size_t length) {
    // Remove pieces until we reach the desired length.
    size_t deleted_len = 0;
    // Because we could be deleting content in the range starting at 'first' where the piece
    // length could be much larger than 'length', we need to adjust 'length' to contain the
    // delta in length within the piece to the end where 'length' starts:
    // "abcd"  "efg"
    //     ^     ^
    //     |_____|
    //      length to delete = 3
    // P1 length: 4
    // P2 length: 3 (though this length does not matter)
    // We're going to remove all of 'P1' and 'P2' in this range and the caller will re-insert
    // these pieces with the correct lengths.  If we fail to adjust 'length' we will delete P1
    // and believe that the entire range was deleted.
    assert(first.node != nullptr);
    auto total_length = first.node->piece.length;
    // std::println("total_length = {}, first.remainder = {}", total_length, first.remainder);
    // (total - remainder) is the section of 'length' where 'first' intersects.
    // length -= (total_length - first.remainder) + total_length;
    length = base::sub_sat(length, (total_length - first.remainder) + total_length);

    auto delete_at_offset = first.start_offset;
    while (deleted_len < length && first.node != nullptr) {
        deleted_len += first.node->piece.length;
        root = root.remove(delete_at_offset);
        first = node_at(&buffers, root, delete_at_offset);
    }
}

void Tree::insert(size_t offset, std::string_view txt) {
    if (txt.empty()) return;
    // This allows us to undo blocks of code.
    if (end_last_insert != offset || root.empty()) {
        append_undo(root, offset);
    }
    internal_insert(offset, txt);
}

void Tree::erase(size_t offset, size_t count) {
    // Rule out the obvious noop.
    if (count == 0 || root.empty()) return;
    append_undo(root, offset);
    internal_remove(offset, count);
}

void Tree::compute_buffer_meta() {
    lf_count = tree_lf_count(root);
    total_content_length = tree_length(root);
}

void Tree::append_undo(const RedBlackTree& old_root, size_t op_offset) {
    // Can't redo if we're creating a new undo entry.
    if (!redo_stack.empty()) {
        redo_stack.clear();
    }
    undo_stack.push_front({.root = old_root, .op_offset = op_offset});
}

UndoRedoResult Tree::try_undo(size_t op_offset) {
    if (undo_stack.empty()) return {.success = false, .op_offset = 0};
    redo_stack.push_front({.root = root, .op_offset = op_offset});
    auto [node, undo_offset] = undo_stack.front();
    root = node;
    undo_stack.pop_front();
    compute_buffer_meta();
    return {.success = true, .op_offset = undo_offset};
}

UndoRedoResult Tree::try_redo(size_t op_offset) {
    if (redo_stack.empty()) return {.success = false, .op_offset = 0};
    undo_stack.push_front({.root = root, .op_offset = op_offset});
    auto [node, redo_offset] = redo_stack.front();
    root = node;
    redo_stack.pop_front();
    compute_buffer_meta();
    return {.success = true, .op_offset = redo_offset};
}

TreeWalker::TreeWalker(const Tree* tree, size_t offset)
    : buffers{&tree->buffers},
      root{tree->root},
      total_content_length{tree->total_content_length},
      stack{{root}},
      total_offset{offset} {
    fast_forward_to(offset);
}

char TreeWalker::next() {
    if (first_ptr == last_ptr) {
        populate_ptrs();
        // If this is exhausted, we're done.
        if (exhausted()) return '\0';
        // Catchall.
        if (first_ptr == last_ptr) return next();
    }
    total_offset++;
    return *first_ptr++;
}

char TreeWalker::current() {
    if (first_ptr == last_ptr) {
        populate_ptrs();
        // If this is exhausted, we're done.
        if (exhausted()) return '\0';
    }
    return *first_ptr;
}

void TreeWalker::seek(size_t offset) {
    stack.clear();
    stack.push_back({root});
    total_offset = offset;
    fast_forward_to(offset);
}

bool TreeWalker::exhausted() const {
    if (stack.empty()) return true;
    // If we have not exhausted the pointers, we're still active.
    if (first_ptr != last_ptr) return false;
    // If there's more than one entry on the stack, we're still active.
    if (stack.size() > 1) return false;
    // Now, if there's exactly one entry and that entry itself is exhausted (no right subtree)
    // we're done.
    auto& entry = stack.back();
    // We descended into a null child, we're done.
    if (entry.node.empty()) return true;
    if (entry.dir == Direction::Right && entry.node.right().empty()) return true;
    return false;
}

size_t TreeWalker::remaining() const {
    return total_content_length - total_offset;
}

void TreeWalker::populate_ptrs() {
    if (exhausted()) return;
    if (stack.back().node.empty()) {
        stack.pop_back();
        populate_ptrs();
        return;
    }

    auto& [node, dir] = stack.back();
    if (dir == Direction::Left) {
        if (!node.left().empty()) {
            auto left = node.left();
            // Change the dir for when we pop back.
            stack.back().dir = Direction::Center;
            stack.push_back({left});
            populate_ptrs();
            return;
        }
        // Otherwise, let's visit the center, we can actually fallthrough.
        stack.back().dir = Direction::Center;
        dir = Direction::Center;
    }

    if (dir == Direction::Center) {
        auto& piece = node.root().piece;
        auto* buffer = buffers->buffer_at(piece.index);
        auto first_offset = buffers->buffer_offset(piece.index, piece.first);
        auto last_offset = buffers->buffer_offset(piece.index, piece.last);
        first_ptr = buffer->buffer.data() + first_offset;
        last_ptr = buffer->buffer.data() + last_offset;
        // Change this direction.
        stack.back().dir = Direction::Right;
        return;
    }

    assert(dir == Direction::Right);
    auto right = node.right();
    stack.pop_back();
    stack.push_back({right});
    populate_ptrs();
}

void TreeWalker::fast_forward_to(size_t offset) {
    auto node = root;
    while (!node.empty()) {
        if (node.root().left_subtree_length > offset) {
            // For when we revisit this node.
            stack.back().dir = Direction::Center;
            node = node.left();
            stack.push_back({node});
        }
        // It is inside this node.
        else if (node.root().left_subtree_length + node.root().piece.length > offset) {
            stack.back().dir = Direction::Right;
            // Make the offset relative to this piece.
            offset -= node.root().left_subtree_length;
            auto& piece = node.root().piece;
            auto* buffer = buffers->buffer_at(piece.index);
            auto first_offset = buffers->buffer_offset(piece.index, piece.first);
            auto last_offset = buffers->buffer_offset(piece.index, piece.last);
            first_ptr = buffer->buffer.data() + first_offset + offset;
            last_ptr = buffer->buffer.data() + last_offset;
            return;
        } else {
            assert(!stack.empty());
            // This parent is no longer relevant.
            stack.pop_back();
            auto offset_amount = node.root().left_subtree_length + node.root().piece.length;
            offset -= offset_amount;
            node = node.right();
            stack.push_back({node});
        }
    }
}

ReverseTreeWalker::ReverseTreeWalker(const Tree* tree, size_t offset)
    : buffers{&tree->buffers}, root{tree->root}, stack{{root}}, total_offset{offset} {
    fast_forward_to(offset);
}

char ReverseTreeWalker::next() {
    if (first_ptr == last_ptr) {
        populate_ptrs();
        // If this is exhausted, we're done.
        if (exhausted()) return '\0';
        // Catchall.
        if (first_ptr == last_ptr) return next();
    }
    // Since CharOffset is unsigned, this will end up wrapping, both 'exhausted' and 'remaining'
    // will return 'true' and '0' respectively.
    total_offset--;
    // A dereference is the pointer value _before_ this actual pointer, just like STL reverse
    // iterator models.
    return *(--first_ptr);
}

char ReverseTreeWalker::current() {
    if (first_ptr == last_ptr) {
        populate_ptrs();
        // If this is exhausted, we're done.
        if (exhausted()) return '\0';
    }
    return *(first_ptr - 1);
}

void ReverseTreeWalker::seek(size_t offset) {
    stack.clear();
    stack.push_back({root});
    total_offset = offset;
    fast_forward_to(offset);
}

bool ReverseTreeWalker::exhausted() const {
    if (stack.empty()) return true;
    // If we have not exhausted the pointers, we're still active.
    if (first_ptr != last_ptr) return false;
    // If there's more than one entry on the stack, we're still active.
    if (stack.size() > 1) return false;
    // Now, if there's exactly one entry and that entry itself is exhausted (no right subtree)
    // we're done.
    auto& entry = stack.back();
    // We descended into a null child, we're done.
    if (entry.node.empty()) return true;
    // Do we need this check for reverse iterators?
    if (entry.dir == Direction::Left && entry.node.left().empty()) return true;
    return false;
}

size_t ReverseTreeWalker::remaining() const {
    return total_offset + 1;
}

void ReverseTreeWalker::populate_ptrs() {
    if (exhausted()) return;
    if (stack.back().node.empty()) {
        stack.pop_back();
        populate_ptrs();
        return;
    }

    auto& [node, dir] = stack.back();
    if (dir == Direction::Right) {
        if (!node.right().empty()) {
            auto right = node.right();
            // Change the dir for when we pop back.
            stack.back().dir = Direction::Center;
            stack.push_back({right});
            populate_ptrs();
            return;
        }
        // Otherwise, let's visit the center, we can actually fallthrough.
        stack.back().dir = Direction::Center;
        dir = Direction::Center;
    }

    if (dir == Direction::Center) {
        auto& piece = node.root().piece;
        auto* buffer = buffers->buffer_at(piece.index);
        auto first_offset = buffers->buffer_offset(piece.index, piece.first);
        auto last_offset = buffers->buffer_offset(piece.index, piece.last);
        last_ptr = buffer->buffer.data() + first_offset;
        first_ptr = buffer->buffer.data() + last_offset;
        // Change this direction.
        stack.back().dir = Direction::Left;
        return;
    }

    assert(dir == Direction::Left);
    auto left = node.left();
    stack.pop_back();
    stack.push_back({left});
    populate_ptrs();
}

void ReverseTreeWalker::fast_forward_to(size_t offset) {
    auto node = root;
    while (!node.empty()) {
        if (node.root().left_subtree_length > offset) {
            assert(!stack.empty());
            // This parent is no longer relevant.
            stack.pop_back();
            node = node.left();
            stack.push_back({node});
        }
        // It is inside this node.
        else if (node.root().left_subtree_length + node.root().piece.length > offset) {
            stack.back().dir = Direction::Left;
            // Make the offset relative to this piece.
            offset -= node.root().left_subtree_length;
            auto& piece = node.root().piece;
            auto* buffer = buffers->buffer_at(piece.index);
            auto first_offset = buffers->buffer_offset(piece.index, piece.first);
            last_ptr = buffer->buffer.data() + first_offset;
            // We extend offset because it is the point where we want to start and because this
            // walker works by dereferencing 'first_ptr - 1', offset + 1 is our 'begin'.
            first_ptr = buffer->buffer.data() + first_offset + (offset + 1);
            return;
        } else {
            // For when we revisit this node.
            stack.back().dir = Direction::Center;
            auto offset_amount = node.root().left_subtree_length + node.root().piece.length;
            offset -= offset_amount;
            node = node.right();
            stack.push_back({node});
        }
    }
}

inline const char* to_string(Color c) {
    switch (c) {
    case Color::Red:
        return "Red";
    case Color::Black:
        return "Black";
    case Color::DoubleBlack:
        return "DoubleBlack";
    }
    return "unknown";
}

void print_piece(const Piece& piece, const Tree* tree, int level) {
    const char* levels = "|||||||||||||||||||||||||||||||";
    printf("%.*sidx{%zd}, first{l{%zd}, c{%zd}}, last{l{%zd}, c{%zd}}, len{%zd}, lf{%zd}\n", level,
           levels, piece.index, piece.first.line, piece.first.column, piece.last.line,
           piece.last.column, piece.length, piece.newline_count);
    auto* buffer = tree->buffers.buffer_at(piece.index);
    auto offset = tree->buffers.buffer_offset(piece.index, piece.first);
    printf("%.*sPiece content: %.*s\n", level, levels, static_cast<int>(piece.length),
           buffer->buffer.data() + offset);
}

void print_tree(const PieceTree::RedBlackTree& root,
                const PieceTree::Tree* tree,
                int level,
                size_t node_offset) {
    if (root.empty()) return;
    const char* levels = "|||||||||||||||||||||||||||||||";
    auto this_offset = node_offset + root.root().left_subtree_length;
    printf("%.*sme: %p, left: %p, right: %p, color: %s\n", level, levels, root.root_ptr(),
           root.left().root_ptr(), root.right().root_ptr(), to_string(root.root_color()));
    print_piece(root.root().piece, tree, level);
    printf("%.*sleft_len{%zd}, left_lf{%zd}, node_offset{%zd}\n", level, levels,
           root.root().left_subtree_length, root.root().left_subtree_lf_count, this_offset);
    printf("\n");
    print_tree(root.left(), tree, level + 1, node_offset);
    printf("\n");
    print_tree(root.right(), tree, level + 1, this_offset + root.root().piece.length);
}

}  // namespace PieceTree
