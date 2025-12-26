#include "base/rand_util.h"
#include "editor/buffer/piece_tree.h"
#include "experiments/fuzz/arg_parser.h"
#include "experiments/fuzz/string_buffer.h"
#include <cstring>
#include <spdlog/spdlog.h>
#include <string>

namespace {
void write_file(const std::string& file_name, const std::string& contents) {
    FILE* fp = fopen(file_name.data(), "wb");
    fwrite(&contents[0], 1, contents.length(), fp);
    fclose(fp);
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
    write_file("fuzz.dot", pt.root().to_graphviz_dot());
}
