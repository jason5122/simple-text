#include "base/filesystem/file_reader.h"
#include "editor_tab.h"

EditorTab::EditorTab(fs::path file_path) : file_path(file_path) {}

void EditorTab::setup(config::ColorScheme& color_scheme) {
    std::string file_contents;
    if (!file_path.empty()) {
        file_contents = ReadFile(file_path);
    }

    buffer.setContents(file_contents);
    highlighter.setLanguage("source.json", color_scheme);

    TSInput input = {&buffer, Buffer::read, TSInputEncodingUTF8};
    highlighter.parse(input);
}
