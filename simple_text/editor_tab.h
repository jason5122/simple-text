#pragma once

#include "base/buffer.h"
#include "base/syntax_highlighter.h"
#include "renderer/movement.h"
#include "renderer/types.h"
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

class EditorTab {
public:
    fs::path file_path;

    base::Buffer buffer;
    base::SyntaxHighlighter highlighter;

    renderer::Point scroll{};

    renderer::CaretInfo start_caret{};
    renderer::CaretInfo end_caret{};

    // TODO: Update this during insertion/deletion.
    int longest_line_x = 0;

    EditorTab(fs::path file_path, renderer::Movement& movement);
    void setup(config::ColorScheme& color_scheme);
    void scrollBuffer(renderer::Point& delta, renderer::Point& max_scroll);

    enum class MovementType {
        kCharacters,
        kLines,
    };

    void moveCaret(MovementType movement_type, bool forward, bool extend);
    void setCaretPosition(renderer::Point pos, bool extend);
    void backspace();

private:
    renderer::Movement& movement;

    static constexpr long long kScrollEventSeparation = 28;
    static constexpr float kUnlockLowerBound = 6;
    static constexpr float kUnlockPercent = 1.9;

    enum class ScrollAxis { Vertical, Horizontal, None };

    std::chrono::time_point<std::chrono::system_clock> last_scroll;
    ScrollAxis axis = ScrollAxis::None;
};
