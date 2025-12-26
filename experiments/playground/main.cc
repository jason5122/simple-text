#include "editor/buffer/piece_tree.h"
#include <spdlog/spdlog.h>

int main() {
    editor::PieceTree pt{"hello"};
    pt.insert(pt.length(), " world");
    pt.insert(2, "!");

    std::string s;
    s.assign("hello");

    s.erase(s.length(), 100);
}
