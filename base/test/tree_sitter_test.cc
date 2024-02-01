#include "third_party/tree_sitter/include/tree_sitter/api.h"
#include "util/file_util.h"
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

    const char* source_code = ReadFile("50k_lines.json");

    buffer buf = {source_code, strlen(source_code)};
    TSInput input = {&buf, read, TSInputEncodingUTF8};

    TSTree* tree = ts_parser_parse(parser, NULL, input);
    TSTree* new_tree = ts_parser_parse(parser, tree, input);

    std::cout << "Hello world!\n";

    return 0;
}
