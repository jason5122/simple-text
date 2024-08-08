#include "base/buffer/buffer.h"
#include "base/filesystem/file_reader.h"
#include "base/syntax_highlighter.h"
#include "util/profile_util.h"
#include <chrono>
#include <gtest/gtest.h>
#include <iostream>
#include <tree_sitter/api.h>

extern "C" TSLanguage* tree_sitter_json();

// struct ReferenceBuffer {
//     std::string data;
//     size_t length;
// };

// const char* ReferenceRead(void* payload, uint32_t byte_index, TSPoint position,
//                           uint32_t* bytes_read) {
//     ReferenceBuffer* buffer = static_cast<ReferenceBuffer*>(payload);
//     *bytes_read = buffer->length - byte_index;
//     return &buffer->data[byte_index];
// }

const char* ReferenceRead(void* payload,
                          uint32_t byte_index,
                          TSPoint position,
                          uint32_t* bytes_read) {
    std::string* buffer = static_cast<std::string*>(payload);
    *bytes_read = buffer->size() - byte_index;
    return &(*buffer)[byte_index];

    // if (byte_index == buffer->size()) {
    //     *bytes_read = 0;
    //     return "";
    // } else {
    //     *bytes_read = 1;
    //     return &(*buffer)[byte_index];
    // }
}

TEST(TreeSitterStringTest, Json10Mb) {
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_json());

    std::string source_code = ReadFile("test_files/10mb.json");

    // size_t length = source_code.size();
    // ReferenceBuffer buffer = {
    //     .data = std::move(source_code),
    //     .length = length,
    // };

    TSInput input = {&source_code, ReferenceRead, TSInputEncodingUTF8};

    TSTree* tree;
    {
        PROFILE_BLOCK_WITH_DURATION("ReferenceBuffer: parse", std::chrono::milliseconds);
        tree = ts_parser_parse(parser, NULL, input);
    }

    {
        volatile int i = 0;  // `volatile` prevents the variable from being optimized out.
        PROFILE_BLOCK_WITH_DURATION("ReferenceBuffer: only read", std::chrono::milliseconds);
        for (const char ch : source_code) {
            i += ch;
        }
    }
}

// TODO: Add actual tests to this test case.
TEST(TreeSitterBufferTest, Json10Mb) {
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_json());

    base::Buffer buffer{ReadFile("test_files/10mb.json")};
    TSInput input = {&buffer, base::SyntaxHighlighter::read, TSInputEncodingUTF8};

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
