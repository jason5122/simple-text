#include "experiments/rope/rope.h"

#include <format>
#include <print>
#include <string>

namespace {

// Small: short enough to see the whole tree in one glance.
rope::Rope example1() { return rope::Rope("Hello, world!"); }

// Medium: a sentence, then a few inserts (front, middle, end) that force
// several leaf and internal splits.
rope::Rope example2() {
    rope::Rope text("The quick brown fox jumps over the lazy dog");
    text.insert(0, "[[");
    text.insert(text.size(), "]]");
    text.insert(text.size() / 2, " ***MIDDLE*** ");
    return text;
}

// Large: enough leaves that the tree grows several levels deep.
rope::Rope example3() {
    std::string text;
    for (int i = 0; i < 500; ++i) {
        text += std::format("word{} ", i);
    }
    return rope::Rope(text);
}

}  // namespace

int main() {
    example1().write_html("rope-small.html");
    example2().write_html("rope-medium.html");
    example3().write_html("rope-large.html");

    std::println("Wrote rope-small.html, rope-medium.html, rope-large.html.");

    return 0;
}
