#include "base/filesystem/file_reader.h"
#include "editor_tab.h"

void EditorTab::setup(fs::path file_path, config::ColorScheme& color_scheme) {
    buffer.setContents(ReadFile(file_path));
    highlighter.setLanguage("source.json", color_scheme);

    TSInput input = {&buffer, Buffer::read, TSInputEncodingUTF8};
    highlighter.parse(input);
}

void EditorTab::setup(config::ColorScheme& color_scheme) {
    buffer.setContents("");
    highlighter.setLanguage("source.json", color_scheme);

    TSInput input = {&buffer, Buffer::read, TSInputEncodingUTF8};
    highlighter.parse(input);
}
