#include "base/rand_util.h"
#include "editor/buffer/piece_tree.h"
#include "experiments/fuzz/arg_parser.h"
#include "experiments/fuzz/string_buffer.h"
#include <cstring>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>

namespace {

void write_file(const std::string& file_name, const std::string& contents) {
    FILE* fp = fopen(file_name.data(), "wb");
    fwrite(&contents[0], 1, contents.length(), fp);
    fclose(fp);
}

std::string to_graphviz_dot(const editor::RedBlackTree& root) {
    std::ostringstream out;
    out << "digraph RB {\n"
           "  node [shape=record, fontname=\"monospace\", fontsize=10];\n"
           "  rankdir=TB;\n";

    if (!root) {
        out << "  empty [label=\"(empty)\"];\n}\n";
        return out.str();
    }

    auto node_id = [](const editor::RedBlackTree& n) -> uintptr_t {
        // Use the piece address as a stable ID (OK for persistent trees).
        return reinterpret_cast<uintptr_t>(&n.piece());
    };

    std::queue<editor::RedBlackTree> q;
    q.push(root);

    while (!q.empty()) {
        auto curr = q.front();
        q.pop();

        uintptr_t id = node_id(curr);

        const auto& p = curr.piece();
        auto color = curr.is_red() ? "red" : "black";
        out << std::format(
            "  n{} [label=\"{{piece.len={} | tree.len={} | tree.lf_count={}}}\", color={}];\n", id,
            p.length, curr.length(), curr.line_feed_count(), color);

        if (auto left = curr.left()) {
            out << std::format("  n{} -> n{} [label=\"L\"];\n", id, node_id(left));
            q.push(left);
        }
        if (auto right = curr.right()) {
            out << std::format("  n{} -> n{} [label=\"R\"];\n", id, node_id(right));
            q.push(right);
        }
    }
    out << "}\n";
    return out.str();
}

}  // namespace

int main(int argc, char** argv) {
    Args args = parse_args(argc, argv);
    editor::PieceTree pt;
    StringBuffer model;

    spdlog::info("ops={}", args.ops);

    for (size_t step = 0; step < args.ops; step++) {
        float chance = base::rand_float();
        // Insert.
        if (chance < 0.9) {
            size_t pos = base::rand_int(0, model.length());
            size_t n = base::rand_int(0, args.payload);
            size_t k = base::rand_int(0, n);
            std::string txt = base::rand_string_with_newlines(n, k);

            pt.insert(pos, txt);
            model.insert(pos, txt);

        }
        // Erase.
        else {
            size_t pos = base::rand_int(0, model.length());
            size_t count = base::rand_int(0, args.payload);

            pt.erase(pos, count);
            model.erase(pos, count);
        }

        // Basic invariants.
        CHECK_EQ(pt.length(), model.length());
        CHECK_EQ(pt.empty(), model.empty());
        CHECK_EQ(pt.line_feed_count(), model.line_feed_count());
        CHECK_EQ(pt.line_count(), model.line_count());

        // Random queries.
        size_t line = base::rand_generator(pt.line_count());
        CHECK_EQ(pt.get_line_content(line), model.get_line_content(line));
        CHECK_EQ(pt.get_line_content_with_newline(line),
                 model.get_line_content_with_newline(line));

        size_t offset = base::rand_int(0, model.length());
        CHECK_EQ(pt.line_at(offset), model.line_at(offset));

        line = base::rand_generator(pt.line_count());
        size_t column = base::rand_int(0, 200);
        CHECK_EQ(pt.offset_at(line, column), model.offset_at(line, column));

        size_t pos = base::rand_int(0, model.length());
        size_t n = base::rand_int(0, 256);
        CHECK_EQ(pt.substr(pos, n), model.substr(pos, n));

        // Expensive full-content check occasionally.
        if (args.check_every != 0 && (step % args.check_every) == 0) {
            CHECK_EQ(pt.str(), model.str());
        }
    }

    spdlog::info("OK ({} ops)", args.ops);
    spdlog::info("lines = {}, length = {}", pt.line_count(), pt.length());
    write_file("fuzz.dot", to_graphviz_dot(pt.root()));
}
