#include "parse_tree.h"

namespace highlight {

ParseTree::~ParseTree() {
    ts_tree_delete(tree);
}

namespace {
constexpr int kBufferLen = 1024;

struct ParseData {
    base::TreeWalker* walker;
    size_t last_offset = 0;
    char buf[kBufferLen + 1];
};

const char* ReadCallback(void* opaque_data, uint32_t offset, TSPoint, uint32_t* bytes_read) {
    ParseData* data = static_cast<ParseData*>(opaque_data);
    // Try to use a cached value if possible.
    if (offset >= data->last_offset and data->walker->offset() > offset) {
        const char* str = data->buf + (offset - data->last_offset);
        *bytes_read = static_cast<uint32_t>(std::strlen(str));
        return str;
    }
    data->last_offset = offset;
    data->walker->seek(offset);
    *bytes_read = 0;
    for (int i = 0; i < kBufferLen && !data->walker->exhausted(); ++i) {
        data->buf[i] = data->walker->next();
        ++*bytes_read;
    }
    data->buf[*bytes_read] = '\0';
    return static_cast<const char*>(data->buf);
};
}  // namespace

void ParseTree::parse(const base::PieceTree& piece_tree, const Language& language) {
    base::TreeWalker walker{&piece_tree};
    ParseData parse_data{&walker};
    TSInput input{
        .payload = &parse_data,
        .read = ReadCallback,
        .encoding = TSInputEncodingUTF8,
    };
    tree = ts_parser_parse(language.getParser(), tree, input);
}

void ParseTree::edit(size_t start_byte, size_t old_end_byte, size_t new_end_byte) {
    TSInputEdit edit = {
        .start_byte = static_cast<uint32_t>(start_byte),
        .old_end_byte = static_cast<uint32_t>(old_end_byte),
        .new_end_byte = static_cast<uint32_t>(new_end_byte),
    };
    ts_tree_edit(tree, &edit);
}

TSTree* ParseTree::getTree() const {
    return tree;
}

}  // namespace highlight
