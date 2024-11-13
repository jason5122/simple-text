#include "base/filesystem/file_reader.h"
#include "syntax_highlighter.h"
#include <wasm.h>
#include <wasmtime.h>

// TODO: Debug use; remove this.
#include "util/profile_util.h"
#include "util/std_print.h"

namespace highlight {

QueryCursor::QueryCursor(size_t start_line, size_t end_line, TSTree* tree, TSQuery* query)
    : cursor{ts_query_cursor_new()} {
    ts_query_cursor_set_point_range(cursor, {static_cast<uint32_t>(start_line), 0},
                                    {static_cast<uint32_t>(end_line + 1), 0});
    TSNode root_node = ts_tree_root_node(tree);
    ts_query_cursor_exec(cursor, query, root_node);
}

QueryCursor::~QueryCursor() {
    ts_query_cursor_delete(cursor);
}

bool QueryCursor::nextMatch(Highlight& h) {
    TSQueryMatch match;
    uint32_t capture_index;
    if (ts_query_cursor_next_capture(cursor, &match, &capture_index)) {
        const TSQueryCapture& capture = match.captures[capture_index];
        const TSNode& node = capture.node;
        h.start = ts_node_start_point(node);
        h.end = ts_node_end_point(node);
        h.capture_index = capture.index;
        return true;
    } else {
        return false;
    }
}

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

namespace {
// static constexpr Rgb kTextColor{51, 51, 51};     // Light.
static constexpr Rgb kTextColor{216, 222, 233};  // Dark.

Rgb ColorForCaptureName(std::string_view name) {
    if (name == "string.special.key") {
        return {249, 174, 88};
    } else if (name == "string") {
        return {128, 185, 121};
    } else if (name == "number") {
        return {249, 174, 88};
    } else if (name == "constant") {
        return {236, 95, 102};
    } else if (name == "escape") {
        return {198, 149, 198};
    } else if (name == "comment") {
        return {128, 128, 128};
    } else if (name == "operator") {
        return {249, 123, 88};
    } else if (name == "function") {
        return {102, 153, 204};
    } else if (name == "type") {
        return {198, 149, 198};
    } else if (name == "keyword") {
        return {198, 149, 198};
    } else if (name == "punctuation.delimiter") {
        return {172, 122, 104};
    } else {
        return kTextColor;
    }
}
}  // namespace

void SyntaxHighlighter::setCppLanguage() {
    loadFromWasm();

    PROFILE_BLOCK("SyntaxHighlighter::setCppLanguage()");
    ts_parser_set_language(parser, json_language);

    uint32_t error_offset = 0;
    TSQueryError error_type = TSQueryErrorNone;
    auto query_path = base::ResourceDir() / "queries/highlights_cpp.scm";
    std::string src = base::ReadFile(query_path.c_str());
    query = ts_query_new(json_language, src.data(), src.length(), &error_offset, &error_type);

    if (error_type != TSQueryErrorNone) {
        std::println("Error creating new TSQuery. error_offset: {}, error type:{} ", error_offset,
                     static_cast<std::underlying_type_t<TSQueryError>>(error_type));
    }

    uint32_t capture_count = ts_query_capture_count(query);
    capture_index_color_table = std::vector(capture_count, kTextColor);
    for (size_t i = 0; i < capture_count; ++i) {
        uint32_t length;
        std::string capture_name = ts_query_capture_name_for_id(query, i, &length);
        std::println("{}: {}", i, capture_name);

        capture_index_color_table[i] = ColorForCaptureName(capture_name);
    }
}

namespace {
constexpr int kBufferLen = 1024;

struct ParseData {
    base::TreeWalker* walker;
    size_t last_offset = 0;
    char buf[kBufferLen + 1];
};

const char* ReadCallback(void* opaque_data, uint32_t offset, TSPoint, uint32_t* bytes_read) {
    ParseData* data = static_cast<ParseData*>(opaque_data);
    // Try to use a cached value if possible.
    if (offset >= data->last_offset and data->walker->offset() > offset) {
        const char* str = data->buf + (offset - data->last_offset);
        *bytes_read = static_cast<uint32_t>(std::strlen(str));
        return str;
    }
    data->last_offset = offset;
    data->walker->seek(offset);
    *bytes_read = 0;
    for (int i = 0; i < kBufferLen && !data->walker->exhausted(); ++i) {
        data->buf[i] = data->walker->next();
        ++*bytes_read;
    }
    data->buf[*bytes_read] = '\0';
    return static_cast<const char*>(data->buf);
};
}  // namespace

void SyntaxHighlighter::parse(const base::PieceTree& piece_tree) {
    base::TreeWalker walker{&piece_tree};
    ParseData parse_data{&walker};
    TSInput input{
        .payload = &parse_data,
        .read = ReadCallback,
        .encoding = TSInputEncodingUTF8,
    };
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

QueryCursor SyntaxHighlighter::startQuery(size_t start_line, size_t end_line) const {
    return {start_line, end_line, tree, query};
}

const Rgb& SyntaxHighlighter::getColor(size_t capture_index) const {
    return capture_index_color_table[capture_index];
}

void SyntaxHighlighter::loadFromWasm() {
    PROFILE_BLOCK("SyntaxHighlighter::loadFromWasm()");

    // fs::path wasm_path = base::ResourceDir() / "wasm/tree-sitter-json.wasm";
    fs::path wasm_path = base::ResourceDir() / "wasm/tree-sitter-cpp.wasm";
    FILE* file = fopen(wasm_path.c_str(), "rb");
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    wasm_byte_vec_t binary;
    wasm_byte_vec_new_uninitialized(&binary, file_size);
    if (fread(binary.data, file_size, 1, file) != 1) {
        std::println("SyntaxHighlighter::loadFromWasm(): Error reading module!");
        return;
    }

    TSWasmError ts_wasm_error;
    json_language =
        ts_wasm_store_load_language(wasm_store, "cpp", binary.data, binary.size, &ts_wasm_error);

    if (!json_language) {
        std::println("SyntaxHighlighter::loadFromWasm() error: Language is null!");
        std::abort();
    }
    assert(ts_language_is_wasm(json_language));

    wasm_byte_vec_delete(&binary);
    fclose(file);
}

}  // namespace highlight
