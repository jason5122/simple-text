#include "third_party/tree_sitter/include/tree_sitter/api.h"
#include "util/file_util_mac.h"
#include <iostream>

extern "C" TSLanguage* tree_sitter_json();

struct buffer {
    const char* buf;
    size_t len;
};

const char* read(void* payload, uint32_t byte_index, TSPoint position, uint32_t* bytes_read) {
    if (byte_index >= ((buffer*)payload)->len) {
        *bytes_read = 0;
        return (char*)"";
    } else {
        *bytes_read = 1;
        return (char*)(((buffer*)payload)->buf) + byte_index;
    }
}

int main() {
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_json());

    // const char* source_code = ReadFile("example.json");
    const char* source_code = R"(
{
  // Test comment.
  "year": 2024,
  "static": true
})";
    buffer buf = {source_code, strlen(source_code)};
    TSInput input = {&buf, read, TSInputEncodingUTF8};

    const char* source_code_edited = R"(
{
  // Test comment.
  "year": 2024,
  "static": true,
  "name": "John Smith"
})";
    buffer buf_edited = {source_code_edited, strlen(source_code_edited)};
    TSInput input_edited = {&buf_edited, read, TSInputEncodingUTF8};

    TSNode root_node;
    TSTree* tree;

    tree = ts_parser_parse(parser, NULL, input);
    root_node = ts_tree_root_node(tree);
    std::cout << ts_node_string(root_node) << '\n';

    TSInputEdit edit = {53, 55, 79, {3, 16}, {4, 0}, {5, 0}};
    ts_tree_edit(tree, &edit);

    tree = ts_parser_parse(parser, tree, input_edited);
    root_node = ts_tree_root_node(tree);
    std::cout << ts_node_string(root_node) << '\n';

    return 0;
}
