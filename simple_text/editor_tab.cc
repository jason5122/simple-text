#include "base/filesystem/file_reader.h"
#include "editor_tab.h"

EditorTab::EditorTab(fs::path file_path)
    : file_path(file_path), last_scroll(std::chrono::system_clock::now()) {}

void EditorTab::setup(config::ColorScheme& color_scheme) {
    std::string file_contents;
    if (!file_path.empty()) {
        file_contents = ReadFile(file_path);
    }

    buffer.setContents(file_contents);
    // highlighter.setLanguage("source.json", color_scheme);

    // TSInput input = {&buffer, SyntaxHighlighter::read, TSInputEncodingUTF8};
    // highlighter.parse(input);
}

// Inspired by Zed's implementation:
// //zed/crates/editor/src/scroll.rs
void EditorTab::scrollBuffer(renderer::Point& delta, renderer::Point& max_scroll) {
    float x = std::abs(delta.x);
    float y = std::abs(delta.y);

    auto now = std::chrono::system_clock::now();
    long long duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_scroll).count();

    if (duration > kScrollEventSeparation) {
        axis = x <= y ? ScrollAxis::Vertical : ScrollAxis::Horizontal;
    } else if (std::max(x, y) >= kUnlockLowerBound) {
        if (axis == ScrollAxis::Vertical) {
            if (x > y && x >= y * kUnlockPercent) {
                axis = ScrollAxis::None;
            }
        } else if (axis == ScrollAxis::Horizontal) {
            if (y > x && y >= x * kUnlockPercent) {
                axis = ScrollAxis::None;
            }
        }
    }

    if (axis == ScrollAxis::Vertical) {
        delta.x = 0;
    } else if (axis == ScrollAxis::Horizontal) {
        delta.y = 0;
    }
    scroll.x = std::clamp(scroll.x + delta.x, 0, max_scroll.x);
    scroll.y = std::clamp(scroll.y + delta.y, 0, max_scroll.y);

    last_scroll = now;
}
