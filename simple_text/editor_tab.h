#pragma once

#include "base/buffer.h"
#include "base/syntax_highlighter.h"
#include "renderer/types.h"
#include <filesystem>

namespace fs = std::filesystem;

class EditorTab {
public:
    fs::path file_path;

    Buffer buffer;
    SyntaxHighlighter highlighter;

    renderer::Point scroll{};

    renderer::CaretInfo start_caret{};
    renderer::CaretInfo end_caret{};

    // TODO: Update this during insertion/deletion.
    float longest_line_x = 0;

    EditorTab(fs::path file_path);
    void setup(config::ColorScheme& color_scheme);
};
