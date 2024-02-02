#include "syntax_highlighter.h"
#include "util/file_util.h"
#include "util/log_util.h"
#include <vector>

extern "C" TSLanguage* tree_sitter_cpp();
extern "C" TSLanguage* tree_sitter_glsl();
extern "C" TSLanguage* tree_sitter_json();
extern "C" TSLanguage* tree_sitter_scheme();

SyntaxHighlighter::SyntaxHighlighter() {
    parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_scheme());

    uint32_t error_offset = 0;
    TSQueryError error_type = TSQueryErrorNone;
    const char* query_code = ReadFile(ResourcePath("highlights_scheme.scm"));
    query = ts_query_new(tree_sitter_scheme(), query_code, strlen(query_code), &error_offset,
                         &error_type);

    if (error_type != TSQueryErrorNone) {
        LogError("SyntaxHighlighter",
                 "Error creating new TSQuery. error_offset: %d, error type: %d", error_offset,
                 error_type);
    }

    std::vector<std::string> capture_names;
    uint32_t capture_count = ts_query_capture_count(query);
    for (int i = 0; i < capture_count; i++) {
        uint32_t length;
        const char* capture_name = ts_query_capture_name_for_id(query, i, &length);
        capture_names.push_back(capture_name);
        LogDefault("SyntaxHighlighter", "capture name %d: %s", i, capture_name);
    }
}

void SyntaxHighlighter::edit(size_t start_byte, size_t old_end_byte, size_t new_end_byte) {
    TSInputEdit edit = {
        static_cast<uint32_t>(start_byte),
        static_cast<uint32_t>(old_end_byte),
        static_cast<uint32_t>(new_end_byte),
        // These are unused!
        {0, 0},
        {0, 0},
        {0, 0},
    };
    ts_tree_edit(tree, &edit);
}
