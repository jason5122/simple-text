#include "language.h"

#include "base/files/file_reader.h"
#include <wasm.h>

// TODO: Debug use; remove this.
#include "util/profile_util.h"

namespace highlight {

Language::Language(wasm_engine_t* engine, std::string_view name)
    : name{name}, parser{ts_parser_new()} {
    TSWasmError ts_wasm_error;
    wasm_store = ts_wasm_store_new(engine, &ts_wasm_error);
    ts_parser_set_wasm_store(parser, wasm_store);
}

Language::~Language() {
    if (parser) ts_parser_take_wasm_store(parser);
    ts_parser_delete(parser);
    ts_language_delete(language);
    ts_query_delete(query);
    ts_wasm_store_delete(wasm_store);
}

Language::Language(Language&& other)
    : name{other.name},
      parser{other.parser},
      wasm_store{other.wasm_store},
      language{other.language},
      query{other.query},
      capture_index_color_table{other.capture_index_color_table} {
    other.parser = nullptr;
    other.wasm_store = nullptr;
    other.language = nullptr;
    other.query = nullptr;
}

Language& Language::operator=(Language&& other) {
    if (&other != this) {
        name = other.name;
        parser = other.parser;
        wasm_store = other.wasm_store;
        language = other.language;
        query = other.query;
        capture_index_color_table = other.capture_index_color_table;
        other.parser = nullptr;
        other.wasm_store = nullptr;
        other.language = nullptr;
        other.query = nullptr;
    }
    return *this;
}

namespace {
// static constexpr Rgb kTextColor{51, 51, 51};     // Light.
static constexpr Rgb kTextColor{216, 222, 233};  // Dark.

constexpr Rgb ColorForCaptureName(std::string_view name);
}  // namespace

void Language::load() {
    {
        PROFILE_BLOCK("SyntaxHighlighter::loadFromWasm()");

        std::string wasm_path =
            fmt::format("{}/languages/{}/language.wasm", base::ResourceDir(), name);

        size_t file_size;
        auto data = base::ReadFileBinary(wasm_path.c_str(), file_size);

        wasm_byte_vec_t binary;
        wasm_byte_vec_new_uninitialized(&binary, file_size);
        binary.data = data.release();  // Release the pointer so we don't double free.

        TSWasmError ts_wasm_error;
        language = ts_wasm_store_load_language(wasm_store, name.data(), binary.data, binary.size,
                                               &ts_wasm_error);

        if (!language) {
            fmt::println("SyntaxHighlighter::loadFromWasm() error: Language is null!");
            std::abort();
        }
        assert(ts_language_is_wasm(language));

        wasm_byte_vec_delete(&binary);
    }

    PROFILE_BLOCK("SyntaxHighlighter::setCppLanguage()");
    ts_parser_set_language(parser, language);

    uint32_t error_offset = 0;
    TSQueryError error_type = TSQueryErrorNone;
    auto query_path = fmt::format("languages/{}/highlights.scm", name);
    std::string src = base::ReadFile(query_path.c_str());
    query = ts_query_new(language, src.data(), src.length(), &error_offset, &error_type);

    if (error_type != TSQueryErrorNone) {
        fmt::println("Error creating new TSQuery. error_offset: {}, error type:{} ", error_offset,
                     static_cast<std::underlying_type_t<TSQueryError>>(error_type));
    }

    uint32_t capture_count = ts_query_capture_count(query);
    capture_index_color_table = std::vector<Rgb>(capture_count);
    for (size_t i = 0; i < capture_count; ++i) {
        uint32_t length;
        std::string capture_name = ts_query_capture_name_for_id(query, i, &length);
        fmt::println("{}: {}", i, capture_name);

        capture_index_color_table[i] = ColorForCaptureName(capture_name);
    }
}

std::vector<Highlight> Language::highlight(TSTree* tree,
                                           size_t start_line,
                                           size_t end_line) const {
    std::vector<Highlight> highlights;

    TSNode root_node = ts_tree_root_node(tree);
    TSQueryCursor* cursor = ts_query_cursor_new();

    ts_query_cursor_exec(cursor, query, root_node);
    ts_query_cursor_set_point_range(cursor, {static_cast<uint32_t>(start_line), 0},
                                    {static_cast<uint32_t>(end_line + 1), 0});

    TSQueryMatch match;
    uint32_t capture_index;
    while (ts_query_cursor_next_capture(cursor, &match, &capture_index)) {
        const TSQueryCapture& capture = match.captures[capture_index];
        const TSNode& node = capture.node;
        TSPoint start = ts_node_start_point(node);
        TSPoint end = ts_node_end_point(node);
        highlights.emplace_back(Highlight{start, end, capture.index});
    }

    ts_query_cursor_delete(cursor);
    return highlights;
}

const Rgb& Language::getColor(size_t capture_index) const {
    return capture_index_color_table[capture_index];
}

TSParser* Language::getParser() const {
    return parser;
}

namespace {
constexpr Rgb ColorForCaptureName(std::string_view name) {
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

}  // namespace highlight
