#include "base/buffer.h"
#include "base/filesystem/file_reader.h"
#include "base/syntax_highlighter.h"
#include "util/profile_util.h"
#include "gtest/gtest.h"
#include <chrono>
#include <iostream>
#include <tree_sitter/api.h>

extern "C" TSLanguage* tree_sitter_json();

struct ReferenceBuffer {
    const char* buffer;
    size_t len;
};

const char* ReadString(void* payload, uint32_t byte_index, TSPoint position,
                       uint32_t* bytes_read) {
    ReferenceBuffer* buf = static_cast<ReferenceBuffer*>(payload);
    size_t len = buf->len;
    *bytes_read = len - byte_index;
    return buf->buffer + byte_index;
}

TEST(TreeSitterStringTest, Json10Mb) {
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_json());

    std::string source_code = ReadFile("test_files/10mb.json");
    ReferenceBuffer buf = {
        .buffer = &source_code[0],
        .len = source_code.size(),
    };
    TSInput input = {&buf, ReadString, TSInputEncodingUTF8};

    TSTree* tree;
    {
        PROFILE_BLOCK_WITH_DURATION("Tree-sitter only parse", std::chrono::milliseconds);
        tree = ts_parser_parse(parser, NULL, input);
    }

    // TSNode root_node = ts_tree_root_node(tree);
    // char* string = ts_node_string(root_node);
    // std::cerr << string << '\n';
}

// TODO: Add actual tests to this test case.
TEST(TreeSitterBufferTest, Json10Mb) {
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_json());

    Buffer buffer;
    buffer.setContents(ReadFile("test_files/10mb.json"));
    TSInput input = {&buffer, SyntaxHighlighter::read, TSInputEncodingUTF8};

    TSTree* tree;
    {
        PROFILE_BLOCK_WITH_DURATION("Tree-sitter only parse", std::chrono::milliseconds);
        tree = ts_parser_parse(parser, NULL, input);
    }

    TSTree* new_tree;
    {
        PROFILE_BLOCK_WITH_DURATION("Tree-sitter re-parse", std::chrono::milliseconds);

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
