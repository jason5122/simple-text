#include "base/filesystem/file_reader.h"
#include "syntax_highlighter.h"
#include <cstdio>
#include <format>
#include <vector>

#include <iostream>

// extern "C" TSLanguage* tree_sitter_c();
// extern "C" TSLanguage* tree_sitter_cpp();
// extern "C" TSLanguage* tree_sitter_glsl();
extern "C" TSLanguage* tree_sitter_json();
// extern "C" TSLanguage* tree_sitter_scheme();

namespace base {

SyntaxHighlighter::SyntaxHighlighter() : parser(ts_parser_new()) {}

SyntaxHighlighter::~SyntaxHighlighter() {
    // This causes segfaults for some reason if SyntaxHighlighter is stored in a std::vector
    // without using std::unique_ptr.
    // https://stackoverflow.com/a/36928283/14698275
    // https://stackoverflow.com/a/21646965/14698275
    ts_parser_delete(parser);
    ts_query_delete(query);
    ts_tree_delete(tree);
}

void SyntaxHighlighter::setLanguage(std::string scope, config::ColorScheme& color_scheme) {
    this->scope = scope;

    TSLanguage* language;
    fs::path highlights_query_filename;

    language = tree_sitter_json();
    highlights_query_filename = "queries/highlights_json.scm";

    // if (scope == "source.scheme") {
    //     language = tree_sitter_scheme();
    //     highlights_query_filename = "queries/highlights_scheme.scm";
    // }
    // else if (scope == "source.json") {
    //     language = tree_sitter_json();
    //     highlights_query_filename = "queries/highlights_json.scm";
    // }
    // else if (scope == "source.c++") {
    //     language = tree_sitter_cpp();
    //     highlights_query_filename = "queries/highlights_cpp.scm";
    // } else if (scope == "source.c") {
    //     language = tree_sitter_c();
    //     highlights_query_filename = "queries/highlights_c.scm";
    // }
    ts_parser_set_language(parser, language);

    uint32_t error_offset = 0;
    TSQueryError error_type = TSQueryErrorNone;
    std::string query_code = ReadFile(ResourceDir() / highlights_query_filename);
    query = ts_query_new(language, &query_code[0], query_code.size(), &error_offset, &error_type);

    if (error_type != TSQueryErrorNone) {
        std::cerr << std::format("Error creating new TSQuery. error_offset: {}, error type: {}\n",
                                 error_offset,
                                 static_cast<std::underlying_type_t<TSQueryError>>(error_type));
    }

    uint32_t capture_count = ts_query_capture_count(query);
    capture_index_color_table = std::vector(capture_count, color_scheme.foreground);
    for (size_t i = 0; i < capture_count; ++i) {
        uint32_t length;
        std::string capture_name = ts_query_capture_name_for_id(query, i, &length);
        // std::cerr << "capture name " << i << ": " << capture_name << '\n';

        if (capture_name == "comment") {
            capture_index_color_table[i] = colors::grey2;
        } else if (capture_name == "string" || capture_name == "string.special.key") {
            capture_index_color_table[i] = colors::green;
        } else if (capture_name == "number") {
            capture_index_color_table[i] = colors::yellow;
        } else if (capture_name == "constant") {
            capture_index_color_table[i] = colors::red;
        } else if (capture_name == "keyword") {
            capture_index_color_table[i] = colors::purple;
        } else if (capture_name == "function") {
            capture_index_color_table[i] = colors::blue;
        } else if (capture_name == "operator") {
            capture_index_color_table[i] = colors::red2;
        } else if (capture_name == "punctuation.delimiter") {
            capture_index_color_table[i] = colors::red3;
        } else if (capture_name == "punctuation.definition") {
            capture_index_color_table[i] = colors::blue2;
        }
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

    idx = 0;
    highlight_ranges.clear();
    capture_indexes.clear();

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

Rgb SyntaxHighlighter::getColor(size_t byte_offset, config::ColorScheme& color_scheme) {
    size_t size = highlight_ranges.size();
    while (idx < size && byte_offset >= highlight_ranges.at(idx).second) {
        ++idx;
    }

    if (idx < size && highlight_ranges.at(idx).first <= byte_offset &&
        byte_offset < highlight_ranges.at(idx).second) {
        size_t capture_index = capture_indexes[idx];
        return capture_index_color_table[capture_index];
    }
    return color_scheme.foreground;
}

}
