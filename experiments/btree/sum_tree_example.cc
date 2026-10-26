#include "experiments/btree/sum_tree.h"
#include "experiments/btree/sum_tree_svg.h"
#include <algorithm>
#include <cstddef>
#include <format>
#include <fstream>
#include <print>
#include <string>
#include <string_view>

namespace {

struct CountSum {
    int count = 0;
    int sum = 0;
    bool operator==(const CountSum&) const = default;
    friend CountSum operator+(const CountSum& a, const CountSum& b) {
        return {a.count + b.count, a.sum + b.sum};
    }
};

struct Number {
    int value = 0;
    CountSum summary() const { return {1, value}; }
};

struct TextSummary {
    size_t bytes = 0;
    size_t lines = 0;      // Newlines.
    size_t last_line = 0;  // Bytes after the last newline, or all of them if there is none.
    bool operator==(const TextSummary&) const = default;
    friend TextSummary operator+(const TextSummary& a, const TextSummary& b) {
        return {a.bytes + b.bytes, a.lines + b.lines,
                b.lines == 0 ? a.last_line + b.last_line : b.last_line};
    }
};

struct Chunk {
    std::string text;

    TextSummary summary() const {
        size_t newline = text.rfind('\n');
        return {text.size(), static_cast<size_t>(std::ranges::count(text, '\n')),
                newline == std::string::npos ? text.size() : text.size() - newline - 1};
    }
};

using Tree = btree::SumTree<Number>;
using Rope = btree::SumTree<Chunk>;

void write(std::string_view path, std::string_view svg) {
    std::ofstream(std::string(path)) << svg;
    std::println("wrote {}", path);
}

struct MaxKey {
    int max = INT_MIN;  // Identity for max.
    bool operator==(const MaxKey&) const = default;
    friend MaxKey operator+(const MaxKey& a, const MaxKey& b) { return {std::max(a.max, b.max)}; }
};

struct Key {
    int key;
    MaxKey summary() const { return {key}; }
};

class BTree {
public:
    bool contains(int k) const {
        auto it = lower_bound(k);
        return it != tree_.end() && it->key == k;
    }
    bool insert(int k) {
        auto it = lower_bound(k);
        if (it != tree_.end() && it->key == k) return false;
        tree_.insert(it, Key{k});
        return true;
    }
    bool remove(int k) {
        auto it = lower_bound(k);
        if (it == tree_.end() || it->key != k) return false;
        tree_.erase(it);
        return true;
    }
    // The keys in order: the tree's items are Keys, so project each one.
    std::vector<int> items() const {
        std::vector<int> out;
        for (const Key& item : tree_) out.push_back(item.key);
        return out;
    }

private:
    // The first item whose key is at least k, since the items are kept sorted.
    btree::SumTree<Key>::iterator lower_bound(int k) const {
        return tree_.seek([k](const MaxKey& s) { return s.max >= k; });
    }
    btree::SumTree<Key> tree_;
};

void example1() {
    BTree tree;
    tree.insert(1);
    tree.insert(5);
    tree.insert(2);
    tree.insert(4);
    tree.insert(3);
    std::println("{}", tree.items());
}

}  // namespace

template <>
struct std::formatter<CountSum> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const CountSum& s, auto& ctx) const {
        return std::format_to(ctx.out(), "({}, {})", s.count, s.sum);
    }
};

template <>
struct std::formatter<Number> : std::formatter<int> {
    auto format(const Number& n, auto& ctx) const {
        return std::formatter<int>::format(n.value, ctx);
    }
};

template <>
struct std::formatter<TextSummary> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const TextSummary& s, auto& ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {})", s.bytes, s.lines, s.last_line);
    }
};

template <>
struct std::formatter<Chunk> : std::formatter<std::string> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = std::formatter<std::string>::parse(ctx);
        set_debug_format();
        return it;
    }
    auto format(const Chunk& chunk, auto& ctx) const {
        return std::formatter<std::string>::format(chunk.text, ctx);
    }
};

int main() {
    example1();

    // The diagram's eight items, appended in order. With t = 2 the eighth append splits the root,
    // and the tree comes out exactly as drawn.
    Tree tree;
    for (int v : {3, 1, 4, 1, 5, 9, 2, 6}) tree.insert(tree.end(), Number{v});
    std::println("whole tree: {}", tree.summary());
    std::println("{}", tree);
    write("sum_tree_example.svg", btree::to_svg(tree));

    // An iterator walks the sequence; start() is the summary of everything before its item.
    for (Tree::iterator c = tree.begin(); c != tree.end(); ++c) {
        std::println("item {} at index {}, after a sum of {}", c->value, c.start().count,
                     c.start().sum);
    }

    // A seek never scans the sequence: it descends by comparing the target with whole subtrees.
    // Along count it answers "which item is third?"...
    Tree::iterator third = tree.seek([](const CountSum& s) { return s.count >= 3; });
    std::println("the 3rd item is {}", third->value);
    // ...and along sum, "where does the running sum reach 15?", in the same O(log n).
    Tree::iterator at15 = tree.seek([](const CountSum& s) { return s.sum >= 15; });
    std::println("the running sum reaches 15 inside {}, after {} items summing to {}", at15->value,
                 at15.start().count, at15.start().sum);

    // An edit re-sums the entries above it, so the same questions get new answers.
    tree.insert(third, Number{100});
    std::println("after inserting 100 before the 3rd item:");
    std::println("{}", tree);
    std::println("the 3rd item is {}, and the running sum reaches 15 inside {}",
                 tree.seek([](const CountSum& s) { return s.count >= 3; })->value,
                 tree.seek([](const CountSum& s) { return s.sum >= 15; })->value);

    // Erasing it again restores every summary on its path.
    tree.erase(tree.seek([](const CountSum& s) { return s.count >= 3; }));
    std::println("after erasing it again: {}", tree.summary());
    std::println("{}", tree);

    // The same tree over text, cut into chunks of eight bytes so that lines straddle chunks.
    std::string_view poem = "the quick\nbrown fox\njumps over\nthe lazy dog\n";
    Rope rope;
    for (size_t i = 0; i < poem.size(); i += 8) {
        rope.insert(rope.end(), Chunk{std::string(poem.substr(i, 8))});
    }
    std::println("{} bytes in {} lines", rope.summary().bytes, rope.summary().lines);
    std::println("{}", rope);
    write("rope_example.svg", btree::to_svg(rope));

    // Byte offset to line and column: seek the chunk holding the offset, then add the summary of
    // the chunk's text before the offset. The result summarizes everything before the offset, so
    // its lines is the line number and its last_line the column.
    size_t offset = 25;
    Rope::iterator chunk = rope.seek([&](const TextSummary& s) { return s.bytes > offset; });
    std::string head = chunk->text.substr(0, offset - chunk.start().bytes);
    TextSummary before = chunk.start() + Chunk{head}.summary();
    std::println("byte {} is on line {} at column {}", offset, before.lines, before.last_line);

    // Line number to byte offset: seek the chunk holding the newline that ends the line before,
    // then step past that newline within the chunk.
    size_t line = 2;
    Rope::iterator at = rope.seek([&](const TextSummary& s) { return s.lines >= line; });
    size_t pos = 0;
    for (size_t k = line - at.start().lines; k > 0; k--) pos = at->text.find('\n', pos) + 1;
    std::println("line {} starts at byte {}", line, at.start().bytes + pos);
}
