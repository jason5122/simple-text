#include "base/filesystem/file_reader.h"
#include "syntax_highlighter.h"
#include <wasm.h>
#include <wasmtime.h>

// TODO: Debug use; remove this.
#include "util/std_print.h"

namespace base {

SyntaxHighlighter::SyntaxHighlighter() : parser{ts_parser_new()}, engine{wasm_engine_new()} {
    TSWasmError ts_wasm_error;
    wasm_store = ts_wasm_store_new(engine, &ts_wasm_error);
    ts_parser_set_wasm_store(parser, wasm_store);
}

SyntaxHighlighter::~SyntaxHighlighter() {
    if (parser) {
        ts_parser_take_wasm_store(parser);
        ts_parser_delete(parser);
    }
    if (query) ts_query_delete(query);
    if (tree) ts_tree_delete(tree);

    if (json_language) ts_language_delete(json_language);
    if (engine) wasm_engine_delete(engine);
    if (wasm_store) ts_wasm_store_delete(wasm_store);
}

void SyntaxHighlighter::setJsonLanguage() {
    loadJsonLanguageFromWasm();
    ts_parser_set_language(parser, json_language);

    uint32_t error_offset = 0;
    TSQueryError error_type = TSQueryErrorNone;
    auto query_path = ResourceDir() / "queries/highlights_json.scm";
    std::string src = base::ReadFile(query_path.c_str());
    query = ts_query_new(json_language, src.data(), src.length(), &error_offset, &error_type);

    if (error_type != TSQueryErrorNone) {
        std::println("Error creating new TSQuery. error_offset: {}, error type:{} ", error_offset,
                     static_cast<std::underlying_type_t<TSQueryError>>(error_type));
    }

    uint32_t capture_count = ts_query_capture_count(query);
    capture_index_color_table = std::vector(capture_count, Rgb{0, 0, 0});
    for (size_t i = 0; i < capture_count; ++i) {
        uint32_t length;
        std::string capture_name = ts_query_capture_name_for_id(query, i, &length);
        std::println("{}: {}", i, capture_name);

        if (capture_name == "string.special.key") {
            capture_index_color_table[i] = {249, 174, 88};
        } else if (capture_name == "string") {
            capture_index_color_table[i] = {128, 185, 121};
        } else if (capture_name == "number") {
            capture_index_color_table[i] = {249, 174, 88};
        } else if (capture_name == "constant.builtin") {
            capture_index_color_table[i] = {236, 95, 102};
        } else if (capture_name == "escape") {
            capture_index_color_table[i] = {198, 149, 198};
        } else if (capture_name == "comment") {
            capture_index_color_table[i] = {128, 128, 128};
        }
    }
}

void SyntaxHighlighter::parse(const TSInput& input) {
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

std::vector<SyntaxHighlighter::Highlight> SyntaxHighlighter::getHighlights(size_t start_line,
                                                                           size_t end_line) const {
    std::vector<Highlight> highlights;

    TSNode root_node = ts_tree_root_node(tree);
    TSQueryCursor* cursor = ts_query_cursor_new();
    for (size_t line = start_line; line <= end_line; line++) {
        ts_query_cursor_exec(cursor, query, root_node);
        // ts_query_cursor_set_point_range(cursor, {static_cast<uint32_t>(start_line), 0},
        //                                 {static_cast<uint32_t>(end_line) + 1, 0});
        ts_query_cursor_set_point_range(cursor, {static_cast<uint32_t>(line), 0},
                                        {static_cast<uint32_t>(line), 150});

        TSQueryMatch match;
        uint32_t capture_index;
        // TODO: Profile this code and optimize it to be as fast as Tree-sitter's CLI.
        while (ts_query_cursor_next_capture(cursor, &match, &capture_index)) {
            const TSQueryCapture& capture = match.captures[capture_index];
            const TSNode& node = capture.node;
            TSPoint start = ts_node_start_point(node);
            TSPoint end = ts_node_end_point(node);
            highlights.emplace_back(start, end, capture.index);
        }
    }
    ts_query_cursor_delete(cursor);
    return highlights;
}

const SyntaxHighlighter::Rgb& SyntaxHighlighter::getColor(size_t capture_index) const {
    return capture_index_color_table[capture_index];
}

void SyntaxHighlighter::loadJsonLanguageFromWasm() {
    fs::path wasm_path = base::ResourceDir() / "wasm/tree-sitter-json.wasm";
    FILE* file = fopen(wasm_path.c_str(), "rb");
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    wasm_byte_vec_t binary;
    wasm_byte_vec_new_uninitialized(&binary, file_size);
    if (fread(binary.data, file_size, 1, file) != 1) {
        std::println("> Error reading module!");
        return;
    }

    TSWasmError ts_wasm_error;
    json_language =
        ts_wasm_store_load_language(wasm_store, "json", binary.data, binary.size, &ts_wasm_error);

    if (!json_language) {
        std::println("language is null!");
        std::abort();
    }
    assert(ts_language_is_wasm(json_language));

    wasm_byte_vec_delete(&binary);
    fclose(file);
}

}
