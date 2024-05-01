#pragma once

#include "base/buffer.h"
#include "base/syntax_highlighter.h"
#include "renderer/types.h"

class EditorTab {
public:
    Buffer buffer;
    SyntaxHighlighter highlighter;

    renderer::Point scroll{};

    renderer::CaretInfo start_caret{};
    renderer::CaretInfo end_caret{};

    // TODO: Update this during insertion/deletion.
    float longest_line_x = 0;

    void setup(fs::path file_path, config::ColorScheme& color_scheme);
};
