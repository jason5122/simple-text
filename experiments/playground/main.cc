#include "editor/buffer/piece_tree.h"
#include <spdlog/spdlog.h>

int main() {
    editor::PieceTree pt{"hello"};
    pt.insert(pt.length(), "\nworld");

    spdlog::info(pt);
    spdlog::info(pt.get_line_range(0));
}
