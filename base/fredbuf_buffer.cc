#include "fredbuf_buffer.h"

void Buffer::setContents(std::string txt) {
    TreeBuilder builder;
    builder.accept(txt);
    piece_tree = builder.create();
}

size_t Buffer::size() {
    return rep(piece_tree.length());
}

size_t Buffer::lineCount() {
    return rep(piece_tree.line_count());
}

void Buffer::getLineContent(std::string* buf, size_t line_index) const {
    piece_tree.get_line_content(buf, Line{line_index + 1});
}

size_t Buffer::byteOfLine(size_t line_index) {
    CharOffset line_start = piece_tree.get_line_range(Line{line_index + 1}).first;
    return rep(line_start);
}

void Buffer::insert(size_t line_index, size_t line_offset, std::string_view txt) {
    CharOffset line_start = piece_tree.get_line_range(Line{line_index + 1}).first;
    piece_tree.insert(line_start + Length{line_offset}, txt);

    debugInfo();
}

void Buffer::remove(size_t line_index, size_t line_offset, size_t bytes) {
    // CharOffset line_start = piece_tree.get_line_range(Line{line_index + 1}).first;
    // piece_tree.remove(line_start + Length{line_offset}, Length{bytes});

    LineRange range = piece_tree.get_line_range(Line{line_index + 1});
    fprintf(stderr, "(%zu, %zu)\n", rep(range.first), rep(range.last));
    if (range.first < range.last) {
        piece_tree.remove(range.first + Length{line_offset}, Length{bytes});
    }

    debugInfo();
}

void Buffer::debugInfo() {
    fprintf(stderr, "size = %zu, lineCount = %zu\n", size(), lineCount());
}
