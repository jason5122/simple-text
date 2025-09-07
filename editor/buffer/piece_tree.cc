#include "base/check.h"
#include "base/functional/scope_exit.h"
#include "base/numeric/saturation_arithmetic.h"
#include "base/unicode/utf8_decoder.h"
#include "editor/buffer/piece_tree.h"
#include "editor/search/aho_corasick.h"
#include <memory>
#include <string>
#include <vector>

namespace editor {

const CharBuffer* BufferCollection::buffer_at(BufferType buffer_type) const {
    return buffer_type == BufferType::Mod ? &mod_buffer : orig_buffer.get();
}

size_t BufferCollection::buffer_offset(BufferType buffer_type, const BufferCursor& cursor) const {
    auto& starts = buffer_at(buffer_type)->line_starts;
    return starts[cursor.line] + cursor.column;
}

namespace {
std::vector<size_t> populate_line_starts(std::string_view buf) {
    std::vector<size_t> starts;
    starts.emplace_back(0);
    const auto len = buf.size();
    for (size_t i = 0; i < len; ++i) {
        char c = buf[i];
        if (c == '\n') {
            starts.emplace_back(i + 1);
        }
    }
    return starts;
}
}  // namespace

PieceTree::PieceTree() : PieceTree("") {}

PieceTree::PieceTree(std::string_view txt) {
    buffers = BufferCollection{
        .orig_buffer = std::make_shared<CharBuffer>(std::string{txt}, populate_line_starts(txt)),
    };

    // In order to maintain the invariant of other buffers, the mod_buffer needs a single
    // line-start of 0.
    buffers.mod_buffer.line_starts.emplace_back(0);
    last_insert = {};

    const auto& buf = *buffers.orig_buffer;
    DCHECK(!buf.line_starts.empty());
    // If this immutable buffer is empty, we can avoid creating a piece for it altogether.
    if (!buf.buffer.empty()) {
        size_t last_line = buf.line_starts.size() - 1;
        // Create a new node that spans this buffer and retains an index to it.
        // Insert the node into the balanced tree.
        Piece piece = {
            .buffer_type = BufferType::Original,
            .first = {.line = 0, .column = 0},
            .last = {.line = last_line, .column = buf.buffer.size() - buf.line_starts[last_line]},
            .length = buf.buffer.size(),
            .newline_count = last_line,
        };
        root = root.insert({piece}, 0);
    }

    compute_buffer_meta();
}

#ifdef TEXTBUF_DEBUG
namespace {
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
    DCHECK_NE(check_black_node_invariant(root), 0);
}
}  // namespace
#endif  // TEXTBUF_DEBUG

void PieceTree::internal_insert(size_t offset, std::string_view txt) {
    DCHECK(!txt.empty());

    base::ScopeExit guard{[&] {
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

    auto result = node_at(offset);
    // If the offset is beyond the buffer, just select the last node.
    if (result.node == nullptr) {
        auto off = base::sub_sat(total_content_length, size_t{1});
        result = node_at(off);
    }

    // There are 3 cases:
    // 1. We are inserting at the beginning of an existing node.
    // 2. We are inserting at the end of an existing node.
    // 3. We are inserting in the middle of the node.
    auto [node, remainder, node_start_offset, line] = result;
    DCHECK_NE(node, nullptr);

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
            auto prev_node_result = node_at(offset - 1);
            if (prev_node_result.node->piece.buffer_type == BufferType::Mod &&
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

    // Case #2.
    const bool inside_node = offset < node_start_offset + node->piece.length;
    if (!inside_node) {
        // There's a bonus case here.  If our last insertion point was the same as this piece's
        // last and it inserted into the mod buffer, then we can simply 'extend' this piece by
        // the following process:
        // 1. Build the new piece.
        // 2. Remove the old piece.
        // 3. Extend the old piece's length to the length of the newly created piece.
        // 4. Re-insert the new piece.
        if (node->piece.buffer_type == BufferType::Mod && node->piece.last == last_insert) {
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
    auto insert_pos = buffer_position(node->piece, remainder);
    auto new_len_right = buffers.buffer_offset(node->piece.buffer_type, node->piece.last) -
                         buffers.buffer_offset(node->piece.buffer_type, insert_pos);
    auto new_piece_right = node->piece;
    new_piece_right.first = insert_pos;
    new_piece_right.length = new_len_right;
    new_piece_right.newline_count =
        line_feed_count(node->piece.buffer_type, insert_pos, node->piece.last);

    // Remove the original node tail.
    auto new_piece_left = trim_piece_right(node->piece, insert_pos);

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

void PieceTree::internal_erase(size_t offset, size_t count) {
    DCHECK(count != 0 && !root.empty());
    base::ScopeExit guard{[&] {
        compute_buffer_meta();
#ifdef TEXTBUF_DEBUG
        satisfies_rb_invariants(root);
#endif  // TEXTBUF_DEBUG
    }};
    auto first = node_at(offset);
    auto last = node_at(offset + count);
    auto first_node = first.node;
    auto last_node = last.node;

    auto start_split_pos = buffer_position(first_node->piece, first.remainder);

    // Simple case: the range of characters we want to delete are
    // held directly within this node.  Remove the node, resize it
    // then add it back.
    if (first_node == last_node) {
        auto end_split_pos = buffer_position(first_node->piece, last.remainder);
        // The removed buffer is somewhere in the middle.  Trim it in both directions.
        auto [left, right] = shrink_piece(first_node->piece, start_split_pos, end_split_pos);

        root = root.remove(first.start_offset);
        // Note: We insert right first so that the 'left' will be inserted to the right node's
        // left.
        if (right.length > 0) root = root.insert({right}, first.start_offset);
        if (left.length > 0) root = root.insert({left}, first.start_offset);
        return;
    }

    // Traverse nodes and delete all nodes within the offset range. First we will build the
    // partial pieces for the nodes that will eventually make up this range.
    // There are four cases here:
    // 1. The entire first node is deleted as well as all of the last node.
    // 2. Part of the first node is deleted and all of the last node.
    // 3. Part of the first node is deleted and part of the last node.
    // 4. The entire first node is deleted and part of the last node.

    auto new_first = trim_piece_right(first_node->piece, start_split_pos);
    if (last_node == nullptr) {
        remove_node_range(first, count);
    } else {
        auto end_split_pos = buffer_position(last_node->piece, last.remainder);
        auto new_last = trim_piece_left(last_node->piece, end_split_pos);
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
size_t PieceTree::accumulate_value(const BufferCollection* buffers,
                                   const Piece& piece,
                                   size_t index) {
    auto* buffer = buffers->buffer_at(piece.buffer_type);
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
size_t PieceTree::accumulate_value_no_lf(const BufferCollection* buffers,
                                         const Piece& piece,
                                         size_t index) {
    auto* buffer = buffers->buffer_at(piece.buffer_type);
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

template <PieceTree::Accumulator accumulate>
void PieceTree::line_start(size_t* offset,
                           const BufferCollection* buffers,
                           const RedBlackTree& node,
                           size_t line) {
    if (node.empty()) return;
    if (line <= node.data().left_subtree_lf_count) {
        line_start<accumulate>(offset, buffers, node.left(), line);
    }
    // The desired line is directly within the node.
    else if (line <= node.data().left_subtree_lf_count + node.data().piece.newline_count) {
        line -= node.data().left_subtree_lf_count;
        size_t len = node.data().left_subtree_length;
        if (line != 0) {
            len += (*accumulate)(buffers, node.data().piece, line - 1);
        }
        *offset += len;

    }
    // Assemble the LHS and RHS.
    else {
        // This case implies that 'left_subtree_lf_count' is strictly < line.
        // The content is somewhere in the middle.
        line -= node.data().left_subtree_lf_count + node.data().piece.newline_count;
        *offset += node.data().left_subtree_length + node.data().piece.length;
        line_start<accumulate>(offset, buffers, node.right(), line);
    }
}

LineRange PieceTree::get_line_range(size_t line) const {
    LineRange range{};
    line_start<&PieceTree::accumulate_value>(&range.first, &buffers, root, line);
    line_start<&PieceTree::accumulate_value_no_lf>(&range.last, &buffers, root, line + 1);
    return range;
}

LineRange PieceTree::get_line_range_with_newline(size_t line) const {
    LineRange range{};
    line_start<&PieceTree::accumulate_value>(&range.first, &buffers, root, line);
    line_start<&PieceTree::accumulate_value>(&range.last, &buffers, root, line + 1);
    return range;
}

std::string PieceTree::str() const {
    std::string str;
    str.reserve(length());
    TreeWalker walker{this};
    while (!walker.exhausted()) {
        str.push_back(walker.next());
    }
    return str;
}

std::string PieceTree::substr(size_t offset, size_t count) const {
    std::string str;
    str.reserve(count);
    TreeWalker walker{this, offset};
    for (size_t i = 0; i < count && !walker.exhausted(); ++i) {
        str.push_back(walker.next());
    }
    return str;
}

std::optional<size_t> PieceTree::find(std::string_view str) const {
    AhoCorasick ac({std::string(str)});
    auto result = ac.match(*this);

    if (result.match_begin == -1) {
        return std::nullopt;
    } else {
        return result.match_begin;
    }
}

size_t PieceTree::length() const { return total_content_length; }

bool PieceTree::empty() const { return total_content_length == 0; }

size_t PieceTree::line_feed_count() const { return lf_count; }

size_t PieceTree::line_count() const { return line_feed_count() + 1; }

size_t PieceTree::line_at(size_t offset) const {
    if (empty()) return 0;
    auto result = node_at(offset);
    return result.line;
}

BufferCursor PieceTree::line_column_at(size_t offset) const {
    if (empty()) return {0, 0};
    auto result = node_at(offset);
    size_t line = result.line;
    auto [first, last] = get_line_range(line);
    size_t col = std::min(offset, last) - first;
    return {line, col};
}

size_t PieceTree::offset_at(size_t line, size_t column) const {
    auto [first, last] = get_line_range(line);
    size_t offset = std::min(first + column, last);
    return offset;
}

std::string PieceTree::get_line_content(size_t line) const {
    if (root.empty()) return "";

    std::string buf;
    size_t line_offset = 0;
    line_start<&PieceTree::accumulate_value>(&line_offset, &buffers, root, line);
    TreeWalker walker{this, line_offset};
    while (!walker.exhausted()) {
        char c = walker.next();
        if (c == '\n') break;
        buf.push_back(c);
    }
    return buf;
}

std::string PieceTree::get_line_content_with_newline(size_t line) const {
    if (root.empty()) return "";

    std::string buf;
    size_t line_offset = 0;
    line_start<&PieceTree::accumulate_value>(&line_offset, &buffers, root, line);
    TreeWalker walker{this, line_offset};
    while (!walker.exhausted()) {
        char c = walker.next();
        buf.push_back(c);
        if (c == '\n') break;
    }
    return buf;
}

std::string PieceTree::get_line_content_for_layout_use(size_t line) const {
    if (root.empty()) return "";

    std::string buf;
    size_t line_offset = 0;
    line_start<&PieceTree::accumulate_value>(&line_offset, &buffers, root, line);
    TreeWalker walker{this, line_offset};
    while (!walker.exhausted()) {
        char c = walker.next();
        if (c == '\n') {
            buf.push_back(' ');
            break;
        } else {
            buf.push_back(c);
        }
    }
    return buf;
}

size_t PieceTree::line_feed_count(BufferType buffer_type,
                                  const BufferCursor& start,
                                  const BufferCursor& end) const {
    // If the end position is the beginning of a new line, then we can just return the difference
    // in lines.
    if (end.column == 0) return end.line - start.line;
    auto& starts = buffers.buffer_at(buffer_type)->line_starts;
    // It means, there is no LF after end.
    if (end.line == starts.size() - 1) return end.line - start.line;
    // Due to the check above, we know that there's at least one more line after 'end.line'.
    auto next_start_offset = starts[end.line + 1];
    auto end_offset = starts[end.line] + end.column;
    // There are more than 1 character after end, which means it can't be LF.
    if (next_start_offset > end_offset + 1) return end.line - start.line;
    // This must be the case.  next_start_offset is a line down, so it is not possible for
    // end_offset to be < it at this point.
    DCHECK_EQ(end_offset + 1, next_start_offset);
    return end.line - start.line;
}

Piece PieceTree::build_piece(std::string_view txt) {
    auto start_offset = buffers.mod_buffer.buffer.size();
    auto scratch_starts = populate_line_starts(txt);
    auto start = last_insert;
    // Offset the new starts relative to the existing buffer.
    for (auto& new_start : scratch_starts) {
        new_start += start_offset;
    }
    // Append new starts.
    // Note: we can drop the first start because the algorithm always adds an empty start.
    auto new_starts_end = scratch_starts.size();
    buffers.mod_buffer.line_starts.reserve(buffers.mod_buffer.line_starts.size() + new_starts_end);
    for (size_t i = 1; i < new_starts_end; ++i) {
        buffers.mod_buffer.line_starts.emplace_back(scratch_starts[i]);
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
    Piece piece = {
        .buffer_type = BufferType::Mod,
        .first = start,
        .last = end_pos,
        .length = end_offset - start_offset,
        .newline_count = line_feed_count(BufferType::Mod, start, end_pos),
    };
    // Update the last insertion.
    last_insert = end_pos;
    return piece;
}

NodePosition PieceTree::node_at(size_t off) const {
    size_t node_start_offset = 0;
    size_t newline_count = 0;

    auto node = root;
    while (!node.empty()) {
        if (off < node.data().left_subtree_length) {
            node = node.left();
        } else if (off < node.data().left_subtree_length + node.data().piece.length) {
            node_start_offset += node.data().left_subtree_length;
            newline_count += node.data().left_subtree_lf_count;
            // Now we find the line within this piece.
            auto remainder = off - node.data().left_subtree_length;
            auto pos = buffer_position(node.data().piece, remainder);
            // Note: since buffer_position will return us a newline relative to the buffer itself,
            // we need to retract it by the starting line of the piece to get the real difference.
            newline_count += pos.line - node.data().piece.first.line;
            return {
                .node = &node.data(),
                .remainder = remainder,
                .start_offset = node_start_offset,
                .line = newline_count,
            };
        } else {
            // If there are no more nodes to traverse to, return this final node.
            if (node.right().empty()) {
                auto offset_amount = node.data().left_subtree_length;
                node_start_offset += offset_amount;
                newline_count +=
                    node.data().left_subtree_lf_count + node.data().piece.newline_count;
                // Now we find the line within this piece.
                auto remainder = node.data().piece.length;
                return {
                    .node = &node.data(),
                    .remainder = remainder,
                    .start_offset = node_start_offset,
                    .line = newline_count,
                };
            }
            auto offset_amount = node.data().left_subtree_length + node.data().piece.length;
            off -= offset_amount;
            node_start_offset += offset_amount;
            newline_count += node.data().left_subtree_lf_count + node.data().piece.newline_count;
            node = node.right();
        }
    }
    return {};
}

BufferCursor PieceTree::buffer_position(const Piece& piece, size_t remainder) const {
    const auto& starts = buffers.buffer_at(piece.buffer_type)->line_starts;
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

Piece PieceTree::trim_piece_right(const Piece& piece, const BufferCursor& pos) const {
    auto orig_end_offset = buffers.buffer_offset(piece.buffer_type, piece.last);

    auto new_end_offset = buffers.buffer_offset(piece.buffer_type, pos);
    auto new_lf_count = line_feed_count(piece.buffer_type, piece.first, pos);

    auto len_delta = orig_end_offset - new_end_offset;
    auto new_len = piece.length - len_delta;

    auto new_piece = piece;
    new_piece.last = pos;
    new_piece.newline_count = new_lf_count;
    new_piece.length = new_len;

    return new_piece;
}

Piece PieceTree::trim_piece_left(const Piece& piece, const BufferCursor& pos) const {
    auto orig_start_offset = buffers.buffer_offset(piece.buffer_type, piece.first);

    auto new_start_offset = buffers.buffer_offset(piece.buffer_type, pos);
    auto new_lf_count = line_feed_count(piece.buffer_type, pos, piece.last);

    auto len_delta = new_start_offset - orig_start_offset;
    auto new_len = piece.length - len_delta;

    auto new_piece = piece;
    new_piece.first = pos;
    new_piece.newline_count = new_lf_count;
    new_piece.length = new_len;

    return new_piece;
}

PieceTree::ShrinkResult PieceTree::shrink_piece(const Piece& piece,
                                                const BufferCursor& first,
                                                const BufferCursor& last) const {
    auto left = trim_piece_right(piece, first);
    auto right = trim_piece_left(piece, last);

    return {.left = left, .right = right};
}

void PieceTree::combine_pieces(NodePosition existing, Piece new_piece) {
    // This transformation is only valid under the following conditions.
    DCHECK_EQ(existing.node->piece.buffer_type, BufferType::Mod);
    // This assumes that the piece was just built.
    DCHECK_EQ(existing.node->piece.last, new_piece.first);
    auto old_piece = existing.node->piece;
    new_piece.first = old_piece.first;
    new_piece.newline_count = new_piece.newline_count + old_piece.newline_count;
    new_piece.length = new_piece.length + old_piece.length;
    root = root.remove(existing.start_offset).insert({new_piece}, existing.start_offset);
}

void PieceTree::remove_node_range(NodePosition first, size_t length) {
    // Remove pieces until we reach the desired length.
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
    DCHECK_NE(first.node, nullptr);
    auto total_length = first.node->piece.length;
    length = length - (total_length - first.remainder) + total_length;

    auto delete_at_offset = first.start_offset;
    size_t deleted_len = 0;
    while (deleted_len < length && first.node != nullptr) {
        deleted_len += first.node->piece.length;
        root = root.remove(delete_at_offset);
        first = node_at(delete_at_offset);
    }
}

void PieceTree::insert(size_t offset, std::string_view txt) {
    if (txt.empty()) return;
    append_undo();
    internal_insert(offset, txt);
}

void PieceTree::erase(size_t offset, size_t count) {
    // Rule out the obvious noop.
    if (count == 0 || root.empty()) return;
    append_undo();
    internal_erase(offset, count);
}

void PieceTree::compute_buffer_meta() {
    lf_count = root.lf_count();
    total_content_length = root.length();
}

void PieceTree::append_undo() {
    // Can't redo if we're creating a new undo entry.
    if (!redo_stack.empty()) {
        redo_stack.clear();
    }
    undo_stack.push_front(root);
}

bool PieceTree::undo() {
    if (undo_stack.empty()) return false;
    redo_stack.push_front(root);
    root = undo_stack.front();
    undo_stack.pop_front();
    compute_buffer_meta();
    return true;
}

bool PieceTree::redo() {
    if (redo_stack.empty()) return false;
    undo_stack.push_front(root);
    root = redo_stack.front();
    redo_stack.pop_front();
    compute_buffer_meta();
    return true;
}

TreeWalker::TreeWalker(const PieceTree* tree, size_t offset)
    : buffers{&tree->buffers},
      root{tree->root},
      total_content_length{tree->total_content_length},
      stack{{root}} {
    total_offset = std::min(offset, tree->length());
    fast_forward_to(total_offset);
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

char32_t TreeWalker::next_codepoint() {
    base::UTF8Decoder decoder;
    while (!exhausted()) {
        decoder.put(next());

        if (decoder.done()) {
            return decoder.value();
        } else if (decoder.error()) {
            return 0;
        }
    }

    if (decoder.done()) {
        return decoder.value();
    } else {
        return 0;
    }
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
        auto& piece = node.data().piece;
        auto* buffer = buffers->buffer_at(piece.buffer_type);
        auto first_offset = buffers->buffer_offset(piece.buffer_type, piece.first);
        auto last_offset = buffers->buffer_offset(piece.buffer_type, piece.last);
        first_ptr = buffer->buffer.data() + first_offset;
        last_ptr = buffer->buffer.data() + last_offset;
        // Change this direction.
        stack.back().dir = Direction::Right;
        return;
    }

    DCHECK_EQ(dir, Direction::Right);
    auto right = node.right();
    stack.pop_back();
    stack.push_back({right});
    populate_ptrs();
}

void TreeWalker::fast_forward_to(size_t offset) {
    auto node = root;
    while (!node.empty()) {
        if (node.data().left_subtree_length > offset) {
            // For when we revisit this node.
            stack.back().dir = Direction::Center;
            node = node.left();
            stack.push_back({node});
        }
        // It is inside this node.
        else if (node.data().left_subtree_length + node.data().piece.length > offset) {
            stack.back().dir = Direction::Right;
            // Make the offset relative to this piece.
            offset -= node.data().left_subtree_length;
            auto& piece = node.data().piece;
            auto* buffer = buffers->buffer_at(piece.buffer_type);
            auto first_offset = buffers->buffer_offset(piece.buffer_type, piece.first);
            auto last_offset = buffers->buffer_offset(piece.buffer_type, piece.last);
            first_ptr = buffer->buffer.data() + first_offset + offset;
            last_ptr = buffer->buffer.data() + last_offset;
            return;
        } else {
            DCHECK(!stack.empty());
            // This parent is no longer relevant.
            stack.pop_back();
            auto offset_amount = node.data().left_subtree_length + node.data().piece.length;
            offset -= offset_amount;
            node = node.right();
            stack.push_back({node});
        }
    }
}

ReverseTreeWalker::ReverseTreeWalker(const PieceTree* tree, size_t offset)
    : buffers{&tree->buffers}, root{tree->root}, stack{{root}} {
    total_offset = std::min(offset, tree->length());
    fast_forward_to(total_offset);
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
    // TODO: Consider changing wrapping behavior.
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

char32_t ReverseTreeWalker::next_codepoint() {
    base::ReverseUTF8Decoder decoder;
    while (!exhausted()) {
        decoder.put(next());

        if (decoder.done()) {
            return decoder.value();
        } else if (decoder.error()) {
            return 0;
        }
    }

    if (decoder.done()) {
        return decoder.value();
    } else {
        return 0;
    }
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
        auto& piece = node.data().piece;
        auto* buffer = buffers->buffer_at(piece.buffer_type);
        auto first_offset = buffers->buffer_offset(piece.buffer_type, piece.first);
        auto last_offset = buffers->buffer_offset(piece.buffer_type, piece.last);
        last_ptr = buffer->buffer.data() + first_offset;
        first_ptr = buffer->buffer.data() + last_offset;
        // Change this direction.
        stack.back().dir = Direction::Left;
        return;
    }

    DCHECK_EQ(dir, Direction::Left);
    auto left = node.left();
    stack.pop_back();
    stack.push_back({left});
    populate_ptrs();
}

void ReverseTreeWalker::fast_forward_to(size_t offset) {
    auto node = root;
    while (!node.empty()) {
        if (node.data().left_subtree_length > offset) {
            DCHECK(!stack.empty());
            // This parent is no longer relevant.
            stack.pop_back();
            node = node.left();
            stack.push_back({node});
        }
        // It is inside this node.
        else if (node.data().left_subtree_length + node.data().piece.length > offset) {
            stack.back().dir = Direction::Left;
            // Make the offset relative to this piece.
            offset -= node.data().left_subtree_length;
            auto& piece = node.data().piece;
            auto* buffer = buffers->buffer_at(piece.buffer_type);
            auto first_offset = buffers->buffer_offset(piece.buffer_type, piece.first);
            last_ptr = buffer->buffer.data() + first_offset;
            first_ptr = buffer->buffer.data() + first_offset + offset;
            return;
        } else {
            // For when we revisit this node.
            stack.back().dir = Direction::Center;
            auto offset_amount = node.data().left_subtree_length + node.data().piece.length;
            offset -= offset_amount;
            node = node.right();
            stack.push_back({node});
        }
    }
}

}  // namespace editor
