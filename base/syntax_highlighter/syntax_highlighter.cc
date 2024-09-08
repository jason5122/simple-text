#include "base/filesystem/file_reader.h"
#include "syntax_highlighter.h"

// TODO: Debug use; remove this.
#include <format>
#include <iostream>

// extern "C" TSLanguage* tree_sitter_c();
// extern "C" TSLanguage* tree_sitter_cpp();
// extern "C" TSLanguage* tree_sitter_glsl();
extern "C" TSLanguage* tree_sitter_json();
// extern "C" TSLanguage* tree_sitter_scheme();

namespace base {

SyntaxHighlighter::SyntaxHighlighter() : parser{ts_parser_new()} {}

SyntaxHighlighter::~SyntaxHighlighter() {
    // This causes segfaults for some reason if SyntaxHighlighter is stored in a std::vector
    // without using std::unique_ptr.
    // https://stackoverflow.com/a/36928283/14698275
    // https://stackoverflow.com/a/21646965/14698275
    ts_parser_delete(parser);
    ts_query_delete(query);
    ts_tree_delete(tree);
}

void SyntaxHighlighter::mystery() {
    TSLanguage* language = tree_sitter_json();
    if (!language) {
        std::cerr << "language is null\n";
    }

    ts_parser_set_language(parser, language);

    uint32_t error_offset = 0;
    TSQueryError error_type = TSQueryErrorNone;
    auto query_path = ResourceDir() / "queries/highlights_json.scm";
    std::string query_code = base::ReadFile(query_path.c_str());
    query =
        ts_query_new(language, query_code.data(), query_code.length(), &error_offset, &error_type);

    if (error_type != TSQueryErrorNone) {
        std::cerr << std::format("Error creating new TSQuery. error_offset: {}, error type:{}\n ",
                                 error_offset,
                                 static_cast<std::underlying_type_t<TSQueryError>>(error_type));
    }

    uint32_t capture_count = ts_query_capture_count(query);
    capture_index_color_table = std::vector(capture_count, Rgb{0, 0, 0});
    for (size_t i = 0; i < capture_count; ++i) {
        uint32_t length;
        std::string capture_name = ts_query_capture_name_for_id(query, i, &length);
        std::cerr << std::format("{}: {}\n", i, capture_name);

        if (capture_name == "number") {
            capture_index_color_table[i] = {255, 0, 0};
        }
    }
}

void SyntaxHighlighter::parse(TSInput& input) {
    tree = ts_parser_parse(parser, tree, input);
}

void SyntaxHighlighter::edit(size_t start_byte, size_t old_end_byte, size_t new_end_byte) {
    TSInputEdit edit = {
        .start_byte = static_cast<uint32_t>(start_byte),
        .old_end_byte = static_cast<uint32_t>(old_end_byte),
        .new_end_byte = static_cast<uint32_t>(new_end_byte),
    };
    ts_tree_edit(tree, &edit);
}

std::vector<SyntaxHighlighter::Highlight> SyntaxHighlighter::getHighlights(size_t start_byte,
                                                                           size_t end_byte) {
    TSNode root_node = ts_tree_root_node(tree);
    TSQueryCursor* query_cursor = ts_query_cursor_new();
    ts_query_cursor_exec(query_cursor, query, root_node);
    ts_query_cursor_set_byte_range(query_cursor, start_byte, end_byte);

    // TODO: Profile this code and optimize it to be as fast as Tree-sitter's CLI.
    const void* prev_id = 0;
    uint32_t prev_start = -1;
    uint32_t prev_end = -1;

    TSQueryMatch match;
    uint32_t capture_index;

    std::vector<Highlight> highlights;
    while (ts_query_cursor_next_capture(query_cursor, &match, &capture_index)) {
        TSQueryCapture capture = match.captures[capture_index];
        TSNode node = capture.node;
        uint32_t start_byte = ts_node_start_byte(node);
        uint32_t end_byte = ts_node_end_byte(node);

        std::cerr << std::format("{}, {}, capture_index = {}\n", start_byte, end_byte,
                                 capture.index);

        if (start_byte != prev_start && end_byte != prev_end && node.id != prev_id) {
            highlights.emplace_back(start_byte, end_byte, capture.index);
        }

        prev_id = node.id;
        prev_start = start_byte;
        prev_end = end_byte;
    }
    return highlights;
}

SyntaxHighlighter::Rgb SyntaxHighlighter::getColor(size_t capture_index) {
    return capture_index_color_table[capture_index];
}

}
