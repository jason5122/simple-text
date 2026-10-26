#pragma once

#include <algorithm>
#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "experiments/btree/sum_tree.h"

namespace btree {

namespace svg {

// A node laid out for drawing: its cells' texts, its children, and once measured and placed,
// its position, the width of its own row of cells, and the span of its whole subtree.
struct Box {
    bool leaf = false;
    std::vector<std::string> cells;
    std::vector<Box> children;
    double x = 0;
    double y = 0;
    double width = 0;
    double span = 0;
};

// Geometry, in SVG user units, after sum_tree_count_and_sum_example.svg.
constexpr double cell_height = 34;
constexpr double row_gap = 70;   // From a cell's bottom to its child's top.
constexpr double node_gap = 20;  // Between neighbouring nodes.
constexpr double margin = 40;
constexpr double top = 50;  // Below the title.

template <typename Node>
Box build(const Node& x) {
    Box box;
    box.leaf = x.leaf;
    for (int i = 0; i < x.n; i++) {
        if (x.leaf) {
            box.cells.push_back(std::format("{}", x.item[i]));
        } else {
            box.cells.push_back(std::format("{}", x.summary[i]));
        }
    }
    if (!x.leaf) {
        for (int i = 0; i < x.n; i++) box.children.push_back(build(*x.c[i]));
    }
    return box;
}

inline size_t longest_text(const Box& box, bool leaf) {
    size_t longest = 0;
    if (box.leaf == leaf) {
        for (const std::string& text : box.cells) longest = std::max(longest, text.size());
    }
    for (const Box& child : box.children) longest = std::max(longest, longest_text(child, leaf));
    return longest;
}

// A cell is as wide as the longest text of its kind needs, at about ten units a character.
inline double cell_width(const Box& root, bool leaf) {
    return std::max(60.0, 10.0 * static_cast<double>(longest_text(root, leaf)) + 20);
}

inline int levels(const Box& box) {
    int deepest = 0;
    for (const Box& child : box.children) deepest = std::max(deepest, levels(child));
    return deepest + 1;
}

inline double children_span(const Box& box) {
    double span = 0;
    for (const Box& child : box.children) span += child.span;
    if (!box.children.empty()) span += node_gap * static_cast<double>(box.children.size() - 1);
    return span;
}

inline void measure(Box& box, double leaf_cell, double nonleaf_cell) {
    box.width = static_cast<double>(box.cells.size()) * (box.leaf ? leaf_cell : nonleaf_cell);
    for (Box& child : box.children) measure(child, leaf_cell, nonleaf_cell);
    box.span = std::max(box.width, children_span(box));
}

// Centres each node over its children, and its children within its span.
inline void place(Box& box, double left, int depth) {
    box.x = left + (box.span - box.width) / 2;
    box.y = top + depth * (cell_height + row_gap);
    double child_left = left + (box.span - children_span(box)) / 2;
    for (Box& child : box.children) {
        place(child, child_left, depth + 1);
        child_left += child.span + node_gap;
    }
}

inline std::string escape(std::string_view text) {
    std::string out;
    for (char c : text) {
        switch (c) {
        case '&':
            out += "&amp;";
            break;
        case '<':
            out += "&lt;";
            break;
        case '>':
            out += "&gt;";
            break;
        case '"':
            out += "&quot;";
            break;
        default:
            out += c;
        }
    }
    return out;
}

// One arrow from the middle of each nonleaf cell to the top of its child, drawn before the cells
// so that the cells cover the arrows' ends.
inline void draw_arrows(const Box& box, std::string& out) {
    double cell = box.width / static_cast<double>(box.cells.size());
    for (size_t i = 0; i < box.children.size(); i++) {
        const Box& child = box.children[i];
        out += std::format(
            "<line x1=\"{}\" y1=\"{}\" x2=\"{}\" y2=\"{}\" marker-end=\"url(#arrow)\"/>\n",
            box.x + (static_cast<double>(i) + 0.5) * cell, box.y + cell_height,
            child.x + child.width / 2, child.y);
        draw_arrows(child, out);
    }
}

inline void draw_cells(const Box& box, std::string& out) {
    double cell = box.width / static_cast<double>(box.cells.size());
    out += std::format("<g class=\"{}\">\n", box.leaf ? "leaf" : "nonleaf");
    for (size_t i = 0; i < box.cells.size(); i++) {
        double x = box.x + static_cast<double>(i) * cell;
        out += std::format("<rect x=\"{}\" y=\"{}\" width=\"{}\" height=\"{}\"/>\n", x, box.y,
                           cell, cell_height);
        out += std::format("<text x=\"{}\" y=\"{}\">{}</text>\n", x + cell / 2,
                           box.y + cell_height / 2, escape(box.cells[i]));
    }
    out += "</g>\n";
    for (const Box& child : box.children) draw_cells(child, out);
}

constexpr std::string_view style = R"svg(<style>
text { font-family: -apple-system, "Helvetica Neue", Helvetica, Arial, sans-serif; font-size: 16px; text-anchor: middle; dominant-baseline: central; }
.title { font-size: 14px; fill: rgb(82, 81, 78); }
line { stroke: rgb(137, 135, 129); stroke-width: 1.5; }
.nonleaf rect { fill: rgb(238, 237, 254); stroke: rgb(83, 74, 183); stroke-width: 0.5; }
.nonleaf text { fill: rgb(60, 52, 137); }
.leaf rect { fill: rgb(225, 245, 238); stroke: rgb(15, 110, 86); stroke-width: 0.5; }
.leaf text { fill: rgb(8, 80, 65); }
</style>
<defs>
<marker id="arrow" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse">
<path d="M2 1L8 5L2 9" fill="none" stroke="rgb(137, 135, 129)" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>
</marker>
</defs>
)svg";

}  // namespace svg

// Draws the tree as sum_tree_count_and_sum_example.svg does: a row of cells per node, nonleaf
// cells holding their child's summary and leaf cells their item, an arrow from each nonleaf
// cell to its child, and the whole tree's summary as a title. Items and summaries print through
// their std::formatter, as they do for std::format("{}", tree).
template <Measured Item, int t>
    requires std::formattable<Item, char> && std::formattable<typename SumTree<Item, t>::S, char>
std::string to_svg(const SumTree<Item, t>& tree) {
    std::string body;
    double width = 2 * svg::margin;
    double height = svg::top + svg::cell_height + svg::margin;
    if (tree.root() != nullptr) {
        svg::Box root = svg::build(*tree.root());
        svg::measure(root, svg::cell_width(root, true), svg::cell_width(root, false));
        svg::place(root, svg::margin, 0);
        width = root.span + 2 * svg::margin;
        height = svg::top + svg::levels(root) * (svg::cell_height + svg::row_gap) - svg::row_gap +
                 svg::margin;
        svg::draw_arrows(root, body);
        svg::draw_cells(root, body);
    }
    std::string out =
        std::format("<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 {} {}\" width=\"{}\" "
                    "height=\"{}\">\n",
                    width, height, width, height);
    out += svg::style;
    out += std::format("<text class=\"title\" x=\"{}\" y=\"{}\">Whole tree: {}</text>\n",
                       width / 2, svg::top - 20, svg::escape(std::format("{}", tree.summary())));
    out += body;
    out += "</svg>\n";
    return out;
}

}  // namespace btree
