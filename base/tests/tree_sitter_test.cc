#include "base/buffer.h"
#include "util/file_util.h"
#include "util/profile_util.h"
#include "gtest/gtest.h"
#include <iostream>
#include <tree_sitter/api.h>

extern "C" TSLanguage* tree_sitter_json();

static const char* read(void* payload, uint32_t byte_index, TSPoint position,
                        uint32_t* bytes_read) {
    Buffer* buffer = (Buffer*)payload;
    if (position.row >= buffer->lineCount()) {
        *bytes_read = 0;
        return "";
    }

    const size_t BUFSIZE = 256;
    static char buf[BUFSIZE];

    std::string line_str;
    buffer->getLineContent(&line_str, position.row);

    size_t len = line_str.size();
    size_t bytes_copied = std::min(len - position.column, BUFSIZE);

    memcpy(buf, &line_str[0] + position.column, bytes_copied);
    *bytes_read = (uint32_t)bytes_copied;
    if (bytes_copied < BUFSIZE) {
        // Add the final \n.
        // If it didn't fit, read() will be called again on the same line with the column advanced.
        buf[bytes_copied] = '\n';
        (*bytes_read)++;
    }
    return buf;
}

// TODO: Add actual tests to this test case.
TEST(TreeSitterTest, Json10Mb) {
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_json());

    Buffer buffer;
    buffer.setContents(ReadFile("test_files/10mb.json"));
    TSInput input = {&buffer, read, TSInputEncodingUTF8};

    TSTree* tree;
    {
        PROFILE_BLOCK("Tree-sitter only parse");
        tree = ts_parser_parse(parser, NULL, input);
    }

    TSTree* new_tree;
    {
        PROFILE_BLOCK("Tree-sitter re-parse");

        buffer.insert(0, 0, "abcdefg");
        TSInputEdit edit = {
            static_cast<uint32_t>(0),
            static_cast<uint32_t>(0),
            static_cast<uint32_t>(7),
            // These are unused!
            {0, 0},
            {0, 0},
            {0, 0},
        };
        ts_tree_edit(tree, &edit);
        new_tree = ts_parser_parse(parser, tree, input);
    }
}
