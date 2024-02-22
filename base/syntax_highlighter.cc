#include "syntax_highlighter.h"
#include "util/file_util.h"
#include "util/profile_util.h"
#include <cstdio>
#include <vector>

extern "C" TSLanguage* tree_sitter_cpp();
extern "C" TSLanguage* tree_sitter_glsl();
extern "C" TSLanguage* tree_sitter_json();
extern "C" TSLanguage* tree_sitter_scheme();

void SyntaxHighlighter::setLanguage(std::string scope) {
    this->scope = scope;

    parser = ts_parser_new();

    TSLanguage* language;
    fs::path highlights_query_filename;
    if (scope == "source.scheme") {
        language = tree_sitter_scheme();
        highlights_query_filename = "queries/highlights_scheme.scm";
    } else if (scope == "source.json") {
        language = tree_sitter_json();
        highlights_query_filename = "queries/highlights_json.scm";
    } else if (scope == "source.c++") {
        language = tree_sitter_cpp();
        highlights_query_filename = "queries/highlights_cpp.scm";
    } else if (scope == "source.c") {
        language = tree_sitter_cpp();
        highlights_query_filename = "queries/highlights_c.scm";
    }
    ts_parser_set_language(parser, language);

    uint32_t error_offset = 0;
    TSQueryError error_type = TSQueryErrorNone;
    std::string query_code = ReadFile(ResourcePath() / highlights_query_filename);
    query = ts_query_new(language, &query_code[0], query_code.size(), &error_offset, &error_type);

    if (error_type != TSQueryErrorNone) {
        fprintf(stderr, "Error creating new TSQuery. error_offset: %d, error type: %d\n",
                error_offset, error_type);
    }

    uint32_t capture_count = ts_query_capture_count(query);
    capture_index_color_table = std::vector(capture_count, BLACK);
    for (size_t i = 0; i < capture_count; i++) {
        uint32_t length;
        const char* capture_name = ts_query_capture_name_for_id(query, i, &length);
        fprintf(stderr, "capture name %zu: %s\n", i, capture_name);
    }
}

void SyntaxHighlighter::parse(TSInput& input) {
    tree = ts_parser_parse(parser, tree, input);
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

void SyntaxHighlighter::getHighlights(TSPoint start_point, TSPoint end_point) {
    if (tree == nullptr) return;

    TSNode root_node = ts_tree_root_node(tree);
    TSQueryCursor* query_cursor = ts_query_cursor_new();
    ts_query_cursor_exec(query_cursor, query, root_node);
    ts_query_cursor_set_point_range(query_cursor, start_point, end_point);

    const void* prev_id = 0;
    uint32_t prev_start = -1;
    uint32_t prev_end = -1;

    // TODO: Profile this code and optimize it to be as fast as Tree-sitter's CLI.
    TSQueryMatch match;
    uint32_t capture_index;

    highlight_ranges.clear();
    capture_indexes.clear();

    {
        PROFILE_BLOCK("while loop of ts_query_cursor_next_capture()");
        while (ts_query_cursor_next_capture(query_cursor, &match, &capture_index)) {
            TSQueryCapture capture = match.captures[capture_index];
            TSNode node = capture.node;
            uint32_t start_byte = ts_node_start_byte(node);
            uint32_t end_byte = ts_node_end_byte(node);

            if (start_byte != prev_start && end_byte != prev_end && node.id != prev_id) {
                highlight_ranges.push_back({start_byte, end_byte});
                capture_indexes.push_back(capture.index);
            }

            prev_id = node.id;
            prev_start = start_byte;
            prev_end = end_byte;
        }
    }
}

bool SyntaxHighlighter::isByteOffsetInRange(size_t byte_offset) {
    if (idx < highlight_ranges.size()) {
        while (byte_offset >= highlight_ranges[idx].second) {
            idx++;
        }

        if (highlight_ranges[idx].first <= byte_offset &&
            byte_offset < highlight_ranges[idx].second) {
            return true;
        }
    }
    return false;
}
