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
        LogError("Renderer", "Error creating new TSQuery. error_offset: %d, error type: %d",
                 error_offset, error_type);
    }

    std::vector<std::string> capture_names;
    uint32_t capture_count = ts_query_capture_count(query);
    for (int i = 0; i < capture_count; i++) {
        uint32_t length;
        const char* capture_name = ts_query_capture_name_for_id(query, i, &length);
        capture_names.push_back(capture_name);
        LogDefault("Renderer", "capture name %d: %s", i, capture_name);
    }
}
