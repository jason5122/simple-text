#include "experiments/rope/rope.h"

#include "base/check.h"
#include <algorithm>
#include <concepts>
#include <cstdint>
#include <format>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace rope {

namespace {

constexpr size_t kMaxLeafLen = 4096;
constexpr size_t kOrder = 8;

// Shape shared by any per-node tree aggregate that isn't a plain sum: a
// leaf-local base case, plus a rule for combining two adjacent subtrees'
// values into their parent's. LineWidthMetric is the first instance; a
// future visual-row-count for soft wrap would be the second, and
// combine_children() below is what lets it reuse this same shape instead of
// another hand-written fold.
template <typename M>
concept Metric = requires(std::string_view text, const M& left, const M& right) {
    { M::of_leaf(text) } -> std::same_as<M>;
    { M::combine(left, right) } -> std::same_as<M>;
};

// Placeholder for a real font/text-shaping backend (Core Text, DirectWrite,
// Pango, ...) -- //experiments/rasterizer isn't wired up yet. Sublime Text
// has the same kind of stand-in (MockUnicodeFont, NullFont) for building/
// testing layout logic before a real backend exists. Deliberately not
// uniform (some characters wider/narrower than others) so a "widest by
// width" test can differ from "longest by character count" and actually
// prove the metric measures width.
double fake_char_width(char c) {
    switch (c) {
    case '\t':
        return 4.0;
    case 'i':
    case 'l':
    case '.':
    case ',':
    case '\'':
        return 0.5;
    case 'm':
    case 'w':
    case 'M':
    case 'W':
        return 1.5;
    default:
        return 1.0;
    }
}

// Unlike length/newline_count, a line's width can't be reduced to a single
// size_t summed by descend<Nav, Acc>, because a line can span a leaf
// boundary. A node caches:
//   - widest_complete_line: the widest line fully contained in this
//     subtree (both ends closed off by a '\n' somewhere in the tree).
//   - leading_open / trailing_open: the width of the still-unclosed run at
//     the very start / end of this subtree's text.
// Combining two children has to consider a line that spans the boundary
// between them (left's trailing_open + right's leading_open) as a
// candidate for the widest line -- the same "does the maximum subarray
// cross the midpoint" case a segment tree has to handle, just applied to a
// B-tree instead of a flat array.
struct LineWidthMetric {
    double widest_complete_line = 0;
    double leading_open = 0;
    double trailing_open = 0;
    size_t newline_count = 0;

    // Base case: measures a leaf's own text directly.
    static LineWidthMetric of_leaf(std::string_view text) {
        LineWidthMetric result;
        double current = 0;
        for (char c : text) {
            if (c == '\n') {
                result.widest_complete_line = std::max(result.widest_complete_line, current);
                if (result.newline_count == 0) {
                    result.leading_open = current;
                }
                ++result.newline_count;
                current = 0;
            } else {
                current += fake_char_width(c);
            }
        }
        result.trailing_open = current;
        if (result.newline_count == 0) {
            result.leading_open = current;
        }
        return result;
    }

    // Combines two adjacent subtrees' metrics into their parent's.
    static LineWidthMetric combine(const LineWidthMetric& left, const LineWidthMetric& right) {
        LineWidthMetric result;
        result.newline_count = left.newline_count + right.newline_count;

        // The run spanning the boundary (left's still-open tail + right's
        // still-open head) is only a genuinely complete line if something
        // closes it off on both sides -- i.e. left contains a '\n' of its
        // own (so left.trailing_open isn't secretly all of left) and
        // likewise for right. Otherwise it's still open and has to
        // propagate outward as this node's own leading/trailing, not get
        // counted as "complete" yet.
        double boundary = left.trailing_open + right.leading_open;
        result.widest_complete_line =
            std::max(left.widest_complete_line, right.widest_complete_line);
        if (left.newline_count > 0 && right.newline_count > 0) {
            result.widest_complete_line = std::max(result.widest_complete_line, boundary);
        }

        result.leading_open = left.newline_count > 0 ? left.leading_open : boundary;
        result.trailing_open = right.newline_count > 0 ? right.trailing_open : boundary;
        return result;
    }
};
static_assert(Metric<LineWidthMetric>);

}  // namespace

// Matches rope.h's forward declaration, so it has to live at rope:: scope
// rather than in the anonymous namespace above. One flat struct for both
// leaf and branch roles -- matching Sublime Text's own text tree
// (TokenBuffer, per disassembly -- see reverse_engineering/node_types.md:
// no vtable, no RTTI, single node type) -- rather than a union/variant of
// the two: `text`/`next` and `children` simply coexist as ordinary fields,
// with only one set meaningful depending on `is_leaf`. That costs every
// node a second field set's worth of space it never uses; the payoff is
// every access is a plain member read, with no accessor indirection and no
// possibility of asking for the wrong alternative. Every helper below only
// needs to be reachable from within this file, so it stays anonymous.
struct Node {
    bool is_leaf;
    size_t length = 0;
    size_t newline_count = 0;
    LineWidthMetric line_width;

    // Leaf-only.
    std::string text;
    Node* next = nullptr;

    // Internal-only.
    std::vector<std::unique_ptr<Node>> children;

    explicit Node(bool leaf) : is_leaf(leaf) {}
};

namespace {

std::unique_ptr<Node> make_leaf() { return std::make_unique<Node>(/*leaf=*/true); }
std::unique_ptr<Node> make_internal() { return std::make_unique<Node>(/*leaf=*/false); }

// Folds a Metric field across `children` left to right via M::combine,
// starting from the first child's own value. The generic counterpart to
// LineWidthMetric::combine's use in recompute_metadata: any future Metric
// field just calls this the same way, rather than another hand-written
// first/combine loop.
template <Metric M, M Node::* Field>
M combine_children(const std::vector<std::unique_ptr<Node>>& children) {
    // recompute_metadata() runs on an internal node even when erase_from()
    // just emptied its last child (the empty check happens after), so this
    // has to tolerate zero children -- matching the old hand-written loop,
    // which likewise left the field default-constructed when it never ran.
    if (children.empty()) {
        return M{};
    }
    M result = children.front().get()->*Field;
    for (size_t i = 1; i < children.size(); ++i) {
        result = M::combine(result, children[i].get()->*Field);
    }
    return result;
}

void recompute_metadata(Node* node) {
    if (node->is_leaf) {
        node->length = node->text.size();
        node->newline_count = std::count(node->text.begin(), node->text.end(), '\n');
        node->line_width = LineWidthMetric::of_leaf(node->text);
        return;
    }
    size_t length = 0;
    size_t newline_count = 0;
    for (const auto& child : node->children) {
        length += child->length;
        newline_count += child->newline_count;
    }
    node->length = length;
    node->newline_count = newline_count;
    node->line_width = combine_children<LineWidthMetric, &Node::line_width>(node->children);
}

// Every "find the leaf for X" query in this file -- leaf_containing,
// find_leaf_and_offset, line_at, line_start_offset -- is the same walk with
// two things swapped: which cached field on Node decides which child to
// descend into (`Nav`), and which field (if any) to sum over every skipped
// sibling along the way (`Acc`). Sublime Text's TokenStorage has a
// near-identical generic summarise<Metric>() for this same reason: it's
// what lets a new per-node metric (e.g. a future visual-row-count for soft
// wrap) get a working query for free instead of another hand-written tree
// walk.
//
// `strict` distinguishes indexing an actual element (leaf_containing: pos
// must land strictly inside a child, via <) from a boundary/insertion-point
// position (insert, line_at: pos may equal a child's cumulative Nav
// exactly and still prefer that earlier child, via <=) -- see
// find_leaf_and_offset's comment below for why the boundary case matters.
//
// On return, `pos` holds the leaf-local remainder and `accumulated` holds
// the sum of `Acc` over every subtree skipped to get there.
template <size_t Node::* Nav, size_t Node::* Acc = nullptr>
const Node* descend(const Node* node, size_t& pos, bool strict, size_t& accumulated) {
    while (!node->is_leaf) {
        const auto& children = node->children;
        size_t chosen = children.size() - 1;
        for (size_t i = 0; i < children.size(); ++i) {
            size_t child_nav = children[i].get()->*Nav;
            if (strict ? (pos < child_nav) : (pos <= child_nav)) {
                chosen = i;
                break;
            }
            pos -= child_nav;
            if constexpr (Acc != nullptr) {
                accumulated += children[i].get()->*Acc;
            }
        }
        node = children[chosen].get();
    }
    return node;
}

// Forward declarations for the mutually-recursive insert helpers.
std::vector<std::unique_ptr<Node>> insert_into(Node* node, size_t pos, std::string_view text);
std::vector<std::unique_ptr<Node>> split_overflowing_internal(Node* node);

// Returns any new right-hand siblings produced by a split, empty if none.
std::vector<std::unique_ptr<Node>> insert_into_leaf(Node* node,
                                                    size_t pos,
                                                    std::string_view text) {
    node->text.insert(pos, text);

    std::vector<std::unique_ptr<Node>> new_leaves;
    if (node->text.size() <= kMaxLeafLen) {
        // Incremental, not recompute_metadata(node): only `text` was added,
        // so counting its newlines is enough -- no need to rescan the
        // whole (up to kMaxLeafLen-character) leaf for a small insert.
        node->length = node->text.size();
        node->newline_count += std::count(text.begin(), text.end(), '\n');
        // line_width isn't incremental like the above: inserting in the
        // middle of an open run changes that run's width regardless of
        // how little text was added, so it needs the whole (still
        // kMaxLeafLen-bounded) leaf rescanned.
        node->line_width = LineWidthMetric::of_leaf(node->text);
        return new_leaves;
    }

    // Overflowed: keep the first chunk in `node`, chunk the rest into new
    // leaves of at most kMaxLeafLen characters, linked into the chain.
    Node* prev = node;
    size_t offset = kMaxLeafLen;
    while (offset < node->text.size()) {
        auto next_leaf = make_leaf();
        next_leaf->text = node->text.substr(offset, kMaxLeafLen);
        next_leaf->next = prev->next;
        prev->next = next_leaf.get();
        prev = next_leaf.get();
        recompute_metadata(next_leaf.get());
        new_leaves.push_back(std::move(next_leaf));
        offset += kMaxLeafLen;
    }
    node->text.resize(kMaxLeafLen);
    recompute_metadata(node);
    return new_leaves;
}

std::vector<std::unique_ptr<Node>> insert_into_internal(Node* node,
                                                        size_t pos,
                                                        std::string_view text) {
    auto& children = node->children;
    size_t running = 0;
    size_t chosen = children.size() - 1;
    for (size_t i = 0; i < children.size(); ++i) {
        if (pos <= running + children[i]->length) {
            chosen = i;
            break;
        }
        running += children[i]->length;
    }

    auto new_children = insert_into(children[chosen].get(), pos - running, text);
    children.insert(children.begin() + chosen + 1, std::make_move_iterator(new_children.begin()),
                    std::make_move_iterator(new_children.end()));
    recompute_metadata(node);
    return split_overflowing_internal(node);
}

std::vector<std::unique_ptr<Node>> insert_into(Node* node, size_t pos, std::string_view text) {
    if (node->is_leaf) {
        return insert_into_leaf(node, pos, text);
    }
    return insert_into_internal(node, pos, text);
}

// Splits `node`'s children into ceil(count / kOrder) balanced groups when
// it has more than kOrder, since a large inserted string can overflow it by
// more than one group's worth at a time. Groups must be balanced, not
// peeled off in fixed kOrder-sized chunks: with fixed-size peeling, the
// common one-over-kOrder overflow leaves `node` with just 1 child, and
// since inserts at the same edge (e.g. repeated appends) keep overflowing
// that same node, the imbalance compounds into a pathologically deep spine.
std::vector<std::unique_ptr<Node>> split_overflowing_internal(Node* node) {
    std::vector<std::unique_ptr<Node>> new_siblings;
    size_t total = node->children.size();
    if (total <= kOrder) {
        return new_siblings;
    }

    std::vector<std::unique_ptr<Node>> all_children = std::move(node->children);
    size_t num_groups = (total + kOrder - 1) / kOrder;
    size_t base_size = total / num_groups;
    size_t remainder = total % num_groups;

    size_t idx = 0;
    for (size_t g = 0; g < num_groups; ++g) {
        size_t group_size = base_size + (g < remainder ? 1 : 0);
        std::unique_ptr<Node> owned_sibling;
        Node* target = node;
        if (g > 0) {
            owned_sibling = make_internal();
            target = owned_sibling.get();
        }
        target->children.assign(std::make_move_iterator(all_children.begin() + idx),
                                std::make_move_iterator(all_children.begin() + idx + group_size));
        recompute_metadata(target);
        if (g > 0) {
            new_siblings.push_back(std::move(owned_sibling));
        }
        idx += group_size;
    }
    return new_siblings;
}

// Leaf containing `pos` (must be < the tree's total length), plus the
// offset of `pos` within its text -- shared by char_at() and leaf_at().
const Node* leaf_containing(const Node* root, size_t pos, size_t& local_pos) {
    size_t unused = 0;
    const Node* leaf = descend<&Node::length>(root, pos, /*strict=*/true, unused);
    local_pos = pos;
    return leaf;
}

// Non-const wrapper around leaf_containing(), for patching a leaf's `next`
// pointer during erase.
Node* leaf_at(Node* root, size_t pos) {
    size_t local_pos = 0;
    return const_cast<Node*>(leaf_containing(root, pos, local_pos));
}

// Same kind of descent as leaf_containing(), but using <= instead of
// strict < so pos == the tree's total length (a valid "one past the end"
// position, e.g. a trailing empty line) still lands somewhere rather than
// silently failing to descend. Used by line_content() to walk the leaf
// chain instead of calling char_at() once per character.
const Node* find_leaf_and_offset(const Node* root, size_t pos, size_t& local_pos) {
    size_t unused = 0;
    const Node* leaf = descend<&Node::length>(root, pos, /*strict=*/false, unused);
    local_pos = pos;
    return leaf;
}

// Absolute offset where `line` begins: the mirror image of line_at's
// descent, walking by newline_count instead of length, then scanning the
// target leaf for the position right after its Nth '\n'.
size_t line_start_offset(const Node* root, size_t line) {
    if (line == 0) {
        return 0;
    }

    size_t target = line;
    size_t offset_before = 0;
    const Node* leaf = descend<&Node::newline_count, &Node::length>(root, target, /*strict=*/false,
                                                                    offset_before);

    // `target` >= 1 here, and the chosen leaf has at least `target` '\n's
    // (that's why it was chosen), so this always finds its Nth one.
    size_t seen = 0;
    for (size_t i = 0; i < leaf->text.size(); ++i) {
        if (leaf->text[i] == '\n' && ++seen == target) {
            return offset_before + i + 1;
        }
    }
    return offset_before;
}

// Erases the local range [pos, pos + count) from `node`'s subtree.
// `left_of_range` is the leaf immediately before the erased range (or null
// if the range starts at position 0); it's threaded through by reference so
// that when an erased leaf empties out, its predecessor can be repointed
// past it, and so the predecessor can be advanced when a leaf survives.
// Returns true if `node` is now empty and should be removed by its caller.
bool erase_from(Node* node, size_t pos, size_t count, Node*& left_of_range) {
    if (node->is_leaf) {
        // Incremental, not recompute_metadata(node): count newlines in just
        // the erased range (before it's gone) instead of rescanning
        // whatever's left of the leaf afterward.
        size_t erased_newlines =
            std::count(node->text.begin() + pos, node->text.begin() + pos + count, '\n');
        node->text.erase(pos, count);
        node->length = node->text.size();
        node->newline_count -= erased_newlines;

        if (node->text.empty()) {
            if (left_of_range) {
                left_of_range->next = node->next;
            }
            return true;
        }
        // Not incremental like the above, for the same reason as the
        // insert side: erasing from the middle of an open run changes
        // that run's width, so line_width needs the surviving text
        // rescanned. Skipped above when the leaf is about to be dropped.
        node->line_width = LineWidthMetric::of_leaf(node->text);
        left_of_range = node;
        return false;
    }

    auto& children = node->children;
    size_t running = 0;
    for (size_t i = 0; i < children.size() && count > 0;) {
        Node* child = children[i].get();
        if (pos >= running + child->length) {
            running += child->length;
            ++i;
            continue;
        }

        size_t local_pos = pos > running ? pos - running : 0;
        size_t local_count = std::min(count, child->length - local_pos);
        bool child_emptied = erase_from(child, local_pos, local_count, left_of_range);
        count -= local_count;

        if (child_emptied) {
            children.erase(children.begin() + i);
        } else {
            running += child->length;
            ++i;
        }
    }
    recompute_metadata(node);
    return children.empty();
}

}  // namespace

Rope::Rope() = default;
Rope::Rope(std::string_view text) { insert(0, text); }
Rope::~Rope() = default;
Rope::Rope(Rope&&) noexcept = default;
Rope& Rope::operator=(Rope&&) noexcept = default;

size_t Rope::size() const { return root_ ? root_->length : 0; }

char Rope::char_at(size_t pos) const {
    CHECK_LT(pos, size());
    size_t local_pos = 0;
    return leaf_containing(root_.get(), pos, local_pos)->text[local_pos];
}

size_t Rope::line_count() const { return (root_ ? root_->newline_count : 0) + 1; }

size_t Rope::line_at(size_t pos) const {
    CHECK_LE(pos, size());
    if (!root_) return 0;

    size_t newlines_before = 0;
    const Node* leaf = descend<&Node::length, &Node::newline_count>(
        root_.get(), pos, /*strict=*/false, newlines_before);
    newlines_before += std::count(leaf->text.begin(), leaf->text.begin() + pos, '\n');
    return newlines_before;
}

std::string Rope::line_content(size_t line) const {
    CHECK_LT(line, line_count());
    if (!root_) return "";

    size_t local_pos = 0;
    const Node* leaf =
        find_leaf_and_offset(root_.get(), line_start_offset(root_.get(), line), local_pos);

    // Walk the leaf chain directly (like str()) instead of calling char_at()
    // once per character: char_at() re-descends from the root every time,
    // which made a single long line as expensive as reading the whole
    // document one character at a time.
    std::string result;
    while (leaf) {
        size_t newline_pos = leaf->text.find('\n', local_pos);
        if (newline_pos != std::string::npos) {
            result.append(leaf->text, local_pos, newline_pos - local_pos);
            return result;
        }
        result.append(leaf->text, local_pos, leaf->text.size() - local_pos);
        leaf = leaf->next;
        local_pos = 0;
    }
    return result;
}

double Rope::widest_line_width() const {
    if (!root_) return 0;
    // leading_open and trailing_open at the root are real lines too (line
    // 0, and the last line whether or not the file ends in '\n') -- they
    // just were never "closed" from both sides, so widest_complete_line
    // alone would miss them.
    const auto& lw = root_->line_width;
    return std::max({lw.widest_complete_line, lw.leading_open, lw.trailing_open});
}

std::string Rope::str() const {
    std::string result;
    if (!root_) return result;
    result.reserve(size());

    const Node* node = root_.get();
    while (!node->is_leaf) {
        node = node->children.front().get();
    }
    for (const Node* leaf = node; leaf; leaf = leaf->next) {
        result += leaf->text;
    }
    return result;
}

void Rope::insert(size_t pos, std::string_view text) {
    CHECK_LE(pos, size());
    if (text.empty()) return;
    if (!root_) root_ = make_leaf();

    auto new_siblings = insert_into(root_.get(), pos, text);
    while (!new_siblings.empty()) {
        auto new_root = make_internal();
        new_root->children.push_back(std::move(root_));
        for (auto& sibling : new_siblings) {
            new_root->children.push_back(std::move(sibling));
        }
        recompute_metadata(new_root.get());
        root_ = std::move(new_root);
        new_siblings = split_overflowing_internal(root_.get());
    }
}

void Rope::erase(size_t pos, size_t count) {
    CHECK_LE(pos + count, size());
    if (count == 0 || !root_) return;

    Node* left_of_range = pos > 0 ? leaf_at(root_.get(), pos - 1) : nullptr;
    if (erase_from(root_.get(), pos, count, left_of_range)) {
        root_.reset();
        return;
    }

    // Collapse a root left with only one child, so a heavily-erased rope
    // doesn't keep a chain of single-child levels above its real content.
    while (!root_->is_leaf && root_->children.size() == 1) {
        root_ = std::move(root_->children.front());
    }
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// Debug
// ─────────────────────────────────────────────────────────────────────────────────────────────────

namespace {

std::string node_id(const Node* node) {
    return std::format("n{:x}", reinterpret_cast<uintptr_t>(node));
}

std::string escape_dot_label(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        switch (c) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += c;
        }
    }
    return out;
}

void append_dot_nodes(std::string& dot, const Node* node) {
    if (node->is_leaf) {
        dot +=
            std::format("  {} [shape=box, style=filled, fillcolor=\"#eef6ff\", label=\"{}\"];\n",
                        node_id(node), escape_dot_label(node->text));
        return;
    }

    dot += std::format(
        "  {} [shape=ellipse, style=filled, fillcolor=\"#fff3e0\", label=\"len={}\"];\n",
        node_id(node), node->length);
    for (const auto& child : node->children) {
        append_dot_nodes(dot, child.get());
    }
}

void append_dot_edges(std::string& dot, const Node* node) {
    if (node->is_leaf) {
        return;
    }
    for (const auto& child : node->children) {
        dot += std::format("  {} -> {};\n", node_id(node), node_id(child.get()));
        append_dot_edges(dot, child.get());
    }
}

// Renders the tree as Graphviz DOT source: solid edges for the tree
// structure, dashed edges tracing the leaf chain. Not part of Rope's public
// API -- write_html() is the intended entry point; this is just how it gets
// there.
std::string to_dot(const Node* root) {
    std::string dot = "digraph rope {\n";
    dot += "  rankdir=TB;\n";
    dot += "  node [fontname=\"monospace\", fontsize=10];\n";
    dot += "  edge [fontname=\"monospace\", fontsize=9];\n";

    if (root) {
        append_dot_nodes(dot, root);
        append_dot_edges(dot, root);

        // Dashed edges tracing the leaf chain -- the same left-to-right
        // linking that str() walks, instead of recursing the tree.
        const Node* node = root;
        while (!node->is_leaf) {
            node = node->children.front().get();
        }
        for (const Node* leaf = node; leaf->next; leaf = leaf->next) {
            dot += std::format("  {} -> {} [style=dashed, color=gray, constraint=false];\n",
                               node_id(leaf), node_id(leaf->next));
        }
    }

    dot += "}\n";
    return dot;
}

constexpr std::string_view kHtmlBefore = R"HTML(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>Rope visualization</title>
<style>
  html, body { margin: 0; height: 100%; font-family: system-ui, sans-serif; }
  #toolbar {
    padding: 8px 12px;
    background: #222;
    color: #eee;
    display: flex;
    gap: 8px;
    align-items: center;
  }
  #toolbar button { cursor: pointer; }
  #viewport {
    width: 100%;
    height: calc(100% - 44px);
    overflow: hidden;
    position: relative;
    cursor: grab;
    background: #fafafa;
  }
  #viewport.dragging { cursor: grabbing; }
  #canvas { transform-origin: 0 0; position: absolute; }
  #canvas svg { display: block; }
</style>
</head>
<body>
  <div id="toolbar">
    <strong>Rope visualization</strong>
    <button id="zoom-in">+</button>
    <button id="zoom-out">-</button>
    <button id="zoom-reset">Reset</button>
    <span>Drag or scroll to pan. Pinch (or Ctrl+scroll) to zoom under the cursor. Solid edges are
    the tree; dashed edges trace the leaf chain.</span>
  </div>
  <div id="viewport">
    <div id="canvas"><p style="padding: 12px; font-family: monospace;">Rendering...</p></div>
  </div>
  <script type="module">
    import { Graphviz } from "https://cdn.jsdelivr.net/npm/@hpcc-js/wasm-graphviz@1.28.0/dist/index.js";

    const dot = )HTML";

constexpr std::string_view kHtmlAfter = R"HTML(;

    const canvas = document.getElementById("canvas");
    const viewport = document.getElementById("viewport");

    let scale = 1, panX = 0, panY = 0;
    const MIN_SCALE = 0.02, MAX_SCALE = 40;

    function applyTransform() {
      canvas.style.transform = `translate(${panX}px, ${panY}px) scale(${scale})`;
    }

    // Zooms by `factor`, keeping the point at (clientX, clientY) fixed on
    // screen -- otherwise zooming always drifts toward the canvas's own
    // (0, 0), not wherever the cursor/pinch actually is.
    function zoomAt(clientX, clientY, factor) {
      const rect = viewport.getBoundingClientRect();
      const x = clientX - rect.left;
      const y = clientY - rect.top;
      const canvasX = (x - panX) / scale;
      const canvasY = (y - panY) / scale;

      scale = Math.min(Math.max(scale * factor, MIN_SCALE), MAX_SCALE);
      panX = x - canvasX * scale;
      panY = y - canvasY * scale;
      applyTransform();
    }

    function viewportCenter() {
      const rect = viewport.getBoundingClientRect();
      return [rect.left + rect.width / 2, rect.top + rect.height / 2];
    }

    // Fits the whole graph in the viewport on first render, since a large
    // rope's tree is far wider than any screen at 1:1 scale.
    function fitToView() {
      const svg = canvas.querySelector("svg");
      if (!svg) return;
      const rect = viewport.getBoundingClientRect();
      const box = svg.getBoundingClientRect();
      const margin = 40;
      scale = Math.min((rect.width - margin) / box.width, (rect.height - margin) / box.height, 1);
      scale = Math.max(scale, MIN_SCALE);
      panX = (rect.width - box.width * scale) / 2;
      panY = (rect.height - box.height * scale) / 2;
      applyTransform();
    }

    document.getElementById("zoom-in").onclick = () => zoomAt(...viewportCenter(), 1.2);
    document.getElementById("zoom-out").onclick = () => zoomAt(...viewportCenter(), 1 / 1.2);
    document.getElementById("zoom-reset").onclick = () => fitToView();

    viewport.addEventListener("wheel", (e) => {
      e.preventDefault();
      // Browsers set metaKey on trackpad pinch gestures (even without a
      // physical Ctrl press), which is the standard way to tell "pinch to
      // zoom" apart from a plain two-finger scroll, which should pan.
      if (e.metaKey) {
        const factor = Math.min(Math.max(Math.exp(-e.deltaY * 0.0075), 0.5), 2);
        zoomAt(e.clientX, e.clientY, factor);
      } else {
        panX -= e.deltaX;
        panY -= e.deltaY;
        applyTransform();
      }
    }, { passive: false });

    let dragging = false, lastX = 0, lastY = 0;
    viewport.addEventListener("mousedown", (e) => {
      dragging = true;
      lastX = e.clientX;
      lastY = e.clientY;
      viewport.classList.add("dragging");
    });
    window.addEventListener("mouseup", () => {
      dragging = false;
      viewport.classList.remove("dragging");
    });
    window.addEventListener("mousemove", (e) => {
      if (!dragging) return;
      panX += e.clientX - lastX;
      panY += e.clientY - lastY;
      lastX = e.clientX;
      lastY = e.clientY;
      applyTransform();
    });

    const graphviz = await Graphviz.load();
    canvas.innerHTML = graphviz.dot(dot, "svg_inline");
    fitToView();
  </script>
</body>
</html>
)HTML";

// Escapes text for embedding as a double-quoted JS string literal inside a
// <script> tag. Every '/' is escaped so a literal "</script>" in the DOT
// source can't close the surrounding tag early.
std::string escape_js_string(std::string_view text) {
    std::string out = "\"";
    for (char c : text) {
        switch (c) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        case '/':
            out += "\\/";
            break;
        default:
            out += c;
        }
    }
    out += "\"";
    return out;
}

}  // namespace

void Rope::write_html(const std::string& path) const {
    std::ofstream file(path);
    file << kHtmlBefore << escape_js_string(to_dot(root_.get())) << kHtmlAfter;
}

}  // namespace rope
