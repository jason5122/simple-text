#include "third_party/tree_sitter/include/tree_sitter/api.h"
#include <stdio.h>
#include <string.h>

TSLanguage* tree_sitter_json();

struct buffer {
    char* buf;
    long len;
};

const char* read_file(void* payload, uint32_t byte_index, TSPoint position, uint32_t* bytes_read) {
    if (byte_index >= ((struct buffer*)payload)->len) {
        *bytes_read = 0;
        return (char*)"";
    } else {
        *bytes_read = 1;
        return (char*)(((struct buffer*)payload)->buf) + byte_index;
    }
}

int main() {
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_json());

    FILE* file = fopen("50k_lines.json", "rb");
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* buffer = malloc(length);
    fread(buffer, 1, length, file);
    fclose(file);

    struct buffer buf = {buffer, length};
    TSInput input = {&buf, read_file, TSInputEncodingUTF8};

    TSTree* tree = ts_parser_parse(parser, NULL, input);
    TSTree* new_tree = ts_parser_parse(parser, tree, input);

    free(buffer);
    ts_tree_delete(tree);
    ts_tree_delete(new_tree);
    ts_parser_delete(parser);

    return 0;
}
