#include "experiments/rope/rope.h"

#include "base/check.h"
#include <algorithm>
#include <cstdint>
#include <format>
#include <fstream>

// A return type written before the qualified function name (e.g. the
// `std::vector<std::unique_ptr<Rope::Node>>` below) is resolved *before*
// the compiler has seen `Rope::`, so it isn't in class scope yet -- private
// nested types used there must be spelled out as `Rope::Node`. Parameter
// types don't have this problem, since they come after the qualified name.

namespace rope {

Rope::Rope(std::string_view text) { insert(0, text); }

size_t Rope::size() const { return root_ ? root_->length : 0; }

double Rope::fake_char_width(char c) {
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

Rope::LineWidthMetric Rope::LineWidthMetric::of_leaf(std::string_view text) {
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

Rope::LineWidthMetric Rope::LineWidthMetric::combine(const LineWidthMetric& left,
                                                     const LineWidthMetric& right) {
    LineWidthMetric result;
    result.newline_count = left.newline_count + right.newline_count;

    // The run spanning the boundary (left's still-open tail + right's
    // still-open head) is only a genuinely complete line if something
    // closes it off on both sides -- i.e. left contains a '\n' of its own
    // (so left.trailing_open isn't secretly all of left) and likewise for
    // right. Otherwise it's still open and has to propagate outward as
    // this node's own leading/trailing, not get counted as "complete" yet.
    double boundary = left.trailing_open + right.leading_open;
    result.widest_complete_line = std::max(left.widest_complete_line, right.widest_complete_line);
    if (left.newline_count > 0 && right.newline_count > 0) {
        result.widest_complete_line = std::max(result.widest_complete_line, boundary);
    }

    result.leading_open = left.newline_count > 0 ? left.leading_open : boundary;
    result.trailing_open = right.newline_count > 0 ? right.trailing_open : boundary;
    return result;
}

template <Metric M, M Rope::Node::* Field>
M Rope::combine_children(const std::vector<std::unique_ptr<Node>>& children) {
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

double Rope::widest_line_width() const {
    if (!root_) {
        return 0;
    }
    // leading_open and trailing_open at the root are real lines too (line
    // 0, and the last line whether or not the file ends in '\n') -- they
    // just were never "closed" from both sides, so widest_complete_line
    // alone would miss them.
    const auto& lw = root_->line_width;
    return std::max({lw.widest_complete_line, lw.leading_open, lw.trailing_open});
}

template <size_t Rope::Node::* Nav, size_t Rope::Node::* Acc>
const Rope::LeafNode* Rope::descend(const Node* node,
                                    size_t& pos,
                                    bool strict,
                                    size_t& accumulated) {
    while (!node->is_leaf) {
        const auto* internal = static_cast<const InternalNode*>(node);
        size_t chosen = internal->children.size() - 1;
        for (size_t i = 0; i < internal->children.size(); ++i) {
            size_t child_nav = internal->children[i].get()->*Nav;
            if (strict ? (pos < child_nav) : (pos <= child_nav)) {
                chosen = i;
                break;
            }
            pos -= child_nav;
            if constexpr (Acc != nullptr) {
                accumulated += internal->children[i].get()->*Acc;
            }
        }
        node = internal->children[chosen].get();
    }
    return static_cast<const LeafNode*>(node);
}

char Rope::char_at(size_t pos) const {
    CHECK_LT(pos, size());
    size_t local_pos = 0;
    return leaf_containing(pos, local_pos)->text[local_pos];
}

size_t Rope::line_count() const { return (root_ ? root_->newline_count : 0) + 1; }

size_t Rope::line_at(size_t pos) const {
    CHECK_LE(pos, size());
    if (!root_) {
        return 0;
    }

    size_t newlines_before = 0;
    const LeafNode* leaf = descend<&Node::length, &Node::newline_count>(
        root_.get(), pos, /*strict=*/false, newlines_before);
    newlines_before += std::count(leaf->text.begin(), leaf->text.begin() + pos, '\n');
    return newlines_before;
}

size_t Rope::line_start_offset(size_t line) const {
    if (line == 0) {
        return 0;
    }

    size_t target = line;
    size_t offset_before = 0;
    const LeafNode* leaf = descend<&Node::newline_count, &Node::length>(
        root_.get(), target, /*strict=*/false, offset_before);

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

const Rope::LeafNode* Rope::find_leaf_and_offset(size_t pos, size_t& local_pos) const {
    // Uses <=, not char_at's strict <: pos == size() is a valid "one past
    // the end" position (line_start_offset returns it for a trailing
    // empty line), and strict < has nothing to match there, silently
    // failing to descend at all and corrupting pos for the next
    // iteration. Landing on the earlier child at an exact boundary is
    // harmless here (unlike char_at, this isn't indexing a character
    // directly) -- line_content resumes from local_pos == that child's
    // length, finds nothing left, and advances via next.
    size_t unused = 0;
    const LeafNode* leaf = descend<&Node::length>(root_.get(), pos, /*strict=*/false, unused);
    local_pos = pos;
    return leaf;
}

std::string Rope::line_content(size_t line) const {
    CHECK_LT(line, line_count());
    if (!root_) {
        return "";
    }

    size_t local_pos = 0;
    const LeafNode* leaf = find_leaf_and_offset(line_start_offset(line), local_pos);

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

std::string Rope::str() const {
    std::string result;
    if (!root_) {
        return result;
    }
    result.reserve(size());

    const Node* node = root_.get();
    while (!node->is_leaf) {
        node = static_cast<const InternalNode*>(node)->children.front().get();
    }
    for (const LeafNode* leaf = static_cast<const LeafNode*>(node); leaf; leaf = leaf->next) {
        result += leaf->text;
    }
    return result;
}

void Rope::insert(size_t pos, std::string_view text) {
    CHECK_LE(pos, size());
    if (text.empty()) {
        return;
    }
    if (!root_) {
        root_ = std::make_unique<LeafNode>();
    }

    auto new_siblings = insert_into(root_.get(), pos, text);
    while (!new_siblings.empty()) {
        auto new_root = std::make_unique<InternalNode>();
        new_root->children.push_back(std::move(root_));
        for (auto& sibling : new_siblings) {
            new_root->children.push_back(std::move(sibling));
        }
        recompute_metadata(new_root.get());
        root_ = std::move(new_root);
        new_siblings = split_overflowing_internal(static_cast<InternalNode*>(root_.get()));
    }
}

void Rope::recompute_metadata(Node* node) {
    if (node->is_leaf) {
        const std::string& text = static_cast<LeafNode*>(node)->text;
        node->length = text.size();
        node->newline_count = std::count(text.begin(), text.end(), '\n');
        node->line_width = LineWidthMetric::of_leaf(text);
        return;
    }
    auto* internal = static_cast<InternalNode*>(node);
    size_t length = 0;
    size_t newline_count = 0;
    for (const auto& child : internal->children) {
        length += child->length;
        newline_count += child->newline_count;
    }
    node->length = length;
    node->newline_count = newline_count;
    node->line_width = combine_children<LineWidthMetric, &Node::line_width>(internal->children);
}

std::vector<std::unique_ptr<Rope::Node>> Rope::insert_into(Node* node,
                                                           size_t pos,
                                                           std::string_view text) {
    if (node->is_leaf) {
        return insert_into_leaf(static_cast<LeafNode*>(node), pos, text);
    }
    return insert_into_internal(static_cast<InternalNode*>(node), pos, text);
}

std::vector<std::unique_ptr<Rope::Node>> Rope::insert_into_leaf(LeafNode* leaf,
                                                                size_t pos,
                                                                std::string_view text) {
    leaf->text.insert(pos, text);

    std::vector<std::unique_ptr<Node>> new_leaves;
    if (leaf->text.size() <= kMaxLeafLen) {
        // Incremental, not recompute_metadata(leaf): only `text` was added,
        // so counting its newlines is enough -- no need to rescan the
        // whole (up to kMaxLeafLen-character) leaf for a small insert.
        leaf->length = leaf->text.size();
        leaf->newline_count += std::count(text.begin(), text.end(), '\n');
        // line_width isn't incremental like the above: inserting in the
        // middle of an open run changes that run's width regardless of
        // how little text was added, so it needs the whole (still
        // kMaxLeafLen-bounded) leaf rescanned.
        leaf->line_width = LineWidthMetric::of_leaf(leaf->text);
        return new_leaves;
    }

    // Overflowed: keep the first chunk in `leaf`, chunk the rest into new
    // leaves of at most kMaxLeafLen characters, linked into the chain.
    LeafNode* prev = leaf;
    size_t offset = kMaxLeafLen;
    while (offset < leaf->text.size()) {
        auto next_leaf = std::make_unique<LeafNode>();
        next_leaf->text = leaf->text.substr(offset, kMaxLeafLen);
        next_leaf->next = prev->next;
        prev->next = next_leaf.get();
        prev = next_leaf.get();
        recompute_metadata(next_leaf.get());
        new_leaves.push_back(std::move(next_leaf));
        offset += kMaxLeafLen;
    }
    leaf->text.resize(kMaxLeafLen);
    recompute_metadata(leaf);
    return new_leaves;
}

std::vector<std::unique_ptr<Rope::Node>> Rope::insert_into_internal(InternalNode* internal,
                                                                    size_t pos,
                                                                    std::string_view text) {
    size_t running = 0;
    size_t chosen = internal->children.size() - 1;
    for (size_t i = 0; i < internal->children.size(); ++i) {
        if (pos <= running + internal->children[i]->length) {
            chosen = i;
            break;
        }
        running += internal->children[i]->length;
    }

    auto new_children = insert_into(internal->children[chosen].get(), pos - running, text);
    internal->children.insert(internal->children.begin() + chosen + 1,
                              std::make_move_iterator(new_children.begin()),
                              std::make_move_iterator(new_children.end()));
    recompute_metadata(internal);
    return split_overflowing_internal(internal);
}

std::vector<std::unique_ptr<Rope::Node>> Rope::split_overflowing_internal(InternalNode* internal) {
    std::vector<std::unique_ptr<Node>> new_siblings;
    size_t total = internal->children.size();
    if (total <= kOrder) {
        return new_siblings;
    }

    // Split into ceil(total / kOrder) groups of as-equal-as-possible size,
    // rather than peeling fixed kOrder-sized chunks off the end: peeling
    // leaves a near-empty remainder on the common one-over-kOrder overflow
    // (5 children -> groups of 1 and 4), and since appends keep overflowing
    // the *same* rightmost node, that 1-vs-4 imbalance compounded every
    // time, making the tree pathologically deep along whichever edge kept
    // growing (invisible in char_at(0), catastrophic in char_at(size - 1)).
    std::vector<std::unique_ptr<Node>> all_children = std::move(internal->children);
    size_t num_groups = (total + kOrder - 1) / kOrder;
    size_t base_size = total / num_groups;
    size_t remainder = total % num_groups;

    size_t idx = 0;
    for (size_t g = 0; g < num_groups; ++g) {
        size_t group_size = base_size + (g < remainder ? 1 : 0);
        std::unique_ptr<Node> owned_sibling;
        InternalNode* target = internal;
        if (g > 0) {
            owned_sibling = std::make_unique<InternalNode>();
            target = static_cast<InternalNode*>(owned_sibling.get());
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

const Rope::LeafNode* Rope::leaf_containing(size_t pos, size_t& local_pos) const {
    size_t unused = 0;
    const LeafNode* leaf = descend<&Node::length>(root_.get(), pos, /*strict=*/true, unused);
    local_pos = pos;
    return leaf;
}

Rope::LeafNode* Rope::leaf_at(size_t pos) {
    size_t local_pos = 0;
    return const_cast<LeafNode*>(leaf_containing(pos, local_pos));
}

void Rope::erase(size_t pos, size_t count) {
    CHECK_LE(pos + count, size());
    if (count == 0 || !root_) {
        return;
    }

    LeafNode* left_of_range = pos > 0 ? leaf_at(pos - 1) : nullptr;
    if (erase_from(root_.get(), pos, count, left_of_range)) {
        root_.reset();
        return;
    }

    // Collapse a root left with only one child, so a heavily-erased rope
    // doesn't keep a chain of single-child levels above its real content.
    while (!root_->is_leaf && static_cast<InternalNode*>(root_.get())->children.size() == 1) {
        root_ = std::move(static_cast<InternalNode*>(root_.get())->children.front());
    }
}

bool Rope::erase_from(Node* node, size_t pos, size_t count, LeafNode*& left_of_range) {
    if (node->is_leaf) {
        auto* leaf = static_cast<LeafNode*>(node);
        // Incremental, not recompute_metadata(leaf): count newlines in just
        // the erased range (before it's gone) instead of rescanning
        // whatever's left of the leaf afterward.
        size_t erased_newlines =
            std::count(leaf->text.begin() + pos, leaf->text.begin() + pos + count, '\n');
        leaf->text.erase(pos, count);
        leaf->length = leaf->text.size();
        leaf->newline_count -= erased_newlines;

        if (leaf->text.empty()) {
            if (left_of_range) {
                left_of_range->next = leaf->next;
            }
            return true;
        }
        // Not incremental like the above, for the same reason as the
        // insert side: erasing from the middle of an open run changes
        // that run's width, so line_width needs the surviving text
        // rescanned. Skipped above when the leaf is about to be dropped.
        leaf->line_width = LineWidthMetric::of_leaf(leaf->text);
        left_of_range = leaf;
        return false;
    }

    auto* internal = static_cast<InternalNode*>(node);
    size_t running = 0;
    for (size_t i = 0; i < internal->children.size() && count > 0;) {
        Node* child = internal->children[i].get();
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
            internal->children.erase(internal->children.begin() + i);
        } else {
            running += child->length;
            ++i;
        }
    }
    recompute_metadata(internal);
    return internal->children.empty();
}

std::string Rope::node_id(const Node* node) {
    return std::format("n{:x}", reinterpret_cast<uintptr_t>(node));
}

std::string Rope::escape_dot_label(std::string_view text) {
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

void Rope::append_dot_nodes(std::string& dot, const Node* node) const {
    if (node->is_leaf) {
        const auto* leaf = static_cast<const LeafNode*>(node);
        dot +=
            std::format("  {} [shape=box, style=filled, fillcolor=\"#eef6ff\", label=\"{}\"];\n",
                        node_id(node), escape_dot_label(leaf->text));
        return;
    }

    dot += std::format(
        "  {} [shape=ellipse, style=filled, fillcolor=\"#fff3e0\", label=\"len={}\"];\n",
        node_id(node), node->length);
    const auto* internal = static_cast<const InternalNode*>(node);
    for (const auto& child : internal->children) {
        append_dot_nodes(dot, child.get());
    }
}

void Rope::append_dot_edges(std::string& dot, const Node* node) const {
    if (node->is_leaf) {
        return;
    }
    const auto* internal = static_cast<const InternalNode*>(node);
    for (const auto& child : internal->children) {
        dot += std::format("  {} -> {};\n", node_id(node), node_id(child.get()));
        append_dot_edges(dot, child.get());
    }
}

std::string Rope::to_dot() const {
    std::string dot = "digraph rope {\n";
    dot += "  rankdir=TB;\n";
    dot += "  node [fontname=\"monospace\", fontsize=10];\n";
    dot += "  edge [fontname=\"monospace\", fontsize=9];\n";

    if (root_) {
        append_dot_nodes(dot, root_.get());
        append_dot_edges(dot, root_.get());

        // Dashed edges tracing the leaf chain -- the same left-to-right
        // linking that str() walks, instead of recursing the tree.
        const Node* node = root_.get();
        while (!node->is_leaf) {
            node = static_cast<const InternalNode*>(node)->children.front().get();
        }
        for (const LeafNode* leaf = static_cast<const LeafNode*>(node); leaf->next;
             leaf = leaf->next) {
            dot += std::format("  {} -> {} [style=dashed, color=gray, constraint=false];\n",
                               node_id(leaf), node_id(leaf->next));
        }
    }

    dot += "}\n";
    return dot;
}

namespace {

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
    file << kHtmlBefore << escape_js_string(to_dot()) << kHtmlAfter;
}

}  // namespace rope
