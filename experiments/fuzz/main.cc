#include "editor/buffer/piece_tree.h"
#include "experiments/fuzz/arg_parser.h"
#include <algorithm>
#include <cstring>
#include <random>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

using editor::PieceTree;

namespace {

template <class RNG>
size_t rand_range(RNG& rng, size_t hi_inclusive) {
    if (hi_inclusive == 0) return 0;
    std::uniform_int_distribution<size_t> dist(0, hi_inclusive);
    return dist(rng);
}

template <class RNG>
size_t biased_pos(RNG& rng, size_t len, size_t last_pos) {
    // 70% uniform, 20% near edges, 10% near last edit
    std::uniform_int_distribution<int> coin(0, 99);
    int r = coin(rng);

    if (r < 70) {
        return rand_range(rng, len);  // inclusive end
    }
    if (r < 90) {
        // edges
        static const size_t kSmall[] = {0, 1, 2, 3, 4};
        if (len <= 4) return rand_range(rng, len);
        std::uniform_int_distribution<int> edge(0, 9);
        int e = edge(rng);
        if (e < 5) return kSmall[e];
        // near end
        size_t back = kSmall[e - 5];
        return (len >= back) ? (len - back) : 0;
    }
    // near last edit
    if (len == 0) return 0;
    std::uniform_int_distribution<int> delta(-16, 16);
    int d = delta(rng);
    int64_t p = static_cast<int64_t>(last_pos) + d;
    if (p < 0) p = 0;
    if (static_cast<size_t>(p) > len) p = static_cast<int64_t>(len);
    return static_cast<size_t>(p);
}

template <class RNG>
std::string rand_text(RNG& rng, size_t n) {
    // Keep it simple but include '\n' to exercise line logic.
    static constexpr char alphabet[] = "abcdefghijklmnopqrstuvwxyz"
                                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                       "0123456789"
                                       " \t"
                                       "\n";

    std::uniform_int_distribution<size_t> pick(0, sizeof(alphabet) - 2);
    std::string s;
    s.reserve(n);
    for (size_t i = 0; i < n; i++) s.push_back(alphabet[pick(rng)]);
    return s;
}

size_t model_line_at(const std::string& m, size_t offset) {
    offset = std::min(offset, m.size());
    size_t line = 0;
    for (size_t i = 0; i < offset; i++) {
        if (m[i] == '\n') line++;
    }
    return line;
}

std::vector<size_t> model_line_starts(const std::string& m) {
    std::vector<size_t> starts;
    starts.push_back(0);
    for (size_t i = 0; i < m.size(); i++) {
        if (m[i] == '\n') starts.push_back(i + 1);
    }
    return starts;
}

size_t model_offset_at(const std::string& m, size_t line, size_t col) {
    auto starts = model_line_starts(m);
    if (starts.empty()) return 0;
    if (line >= starts.size()) line = starts.size() - 1;

    size_t line_start = starts[line];
    // line end is either next start - 1, or end
    size_t line_end = (line + 1 < starts.size()) ? (starts[line + 1] - 1) : m.size();
    size_t line_len = (line_end >= line_start) ? (line_end - line_start) : 0;
    col = std::min(col, line_len);
    return line_start + col;
}

[[noreturn]] void fail(const Args& a,
                       uint64_t seed,
                       size_t step,
                       const std::string& why,
                       const PieceTree& pt,
                       const std::string& model,
                       const std::vector<std::string>& log) {
    spdlog::error("\n=== FAIL ===\n");
    spdlog::error("seed={} step={}", seed, step);
    spdlog::error("why: {}", why);
    spdlog::error("pt.length={} model.length={}\n", pt.length(), model.length());

    spdlog::error("--- last ops ({}) ---", log.size());
    for (const auto& s : log) spdlog::error(s);

    // Helpful if pt.str() is expensive; still nice to show on failures.
    spdlog::error("--- pt.str() ---");
    spdlog::error(pt.str());
    spdlog::error("--- model ---");
    spdlog::error(model);
    std::exit(1);
}

}  // namespace

int main(int argc, char** argv) {
    Args args = parse_args(argc, argv);

    uint64_t seed = args.seed;
    if (!args.seed_provided) {
        std::random_device rd;
        seed = (static_cast<uint64_t>(rd()) << 32) ^ static_cast<uint64_t>(rd());
    }

    std::mt19937_64 rng(seed);

    PieceTree pt;
    std::string model;

    // Model undo/redo stacks (simple but correct).
    std::vector<std::string> undo_stack;
    std::vector<std::string> redo_stack;

    std::vector<std::string> log;
    log.reserve(args.log_keep);

    size_t last_pos = 0;

    spdlog::info("PieceTree randomized harness");
    spdlog::info("seed={} ops={}", seed, args.ops);

    auto push_log = [&](std::string s) {
        if (args.log_keep == 0) return;
        if (log.size() == args.log_keep) log.erase(log.begin());
        log.push_back(std::move(s));
    };

    auto checkpoint_for_edit = [&] {
        undo_stack.push_back(model);
        redo_stack.clear();
    };

    for (size_t step = 0; step < args.ops; step++) {
        // Pick an operation.
        // 0 insert, 1 erase, 2 replace, 3 undo, 4 redo, 5 query-only, 6 clear
        std::uniform_int_distribution<int> opdist(0, 99);
        int r = opdist(rng);

        int op = 0;
        if (r < 45) op = 0;       // insert
        else if (r < 75) op = 1;  // erase
        else if (r < 90) op = 2;  // replace
        // else if (r < 93) op = 3;  // undo
        // else if (r < 96) op = 4;  // redo
        else if (r < 99) op = 5;  // queries
        else op = 6;              // clear

        size_t len = model.size();

        // Occasionally force bigger payloads.
        auto pick_payload_len = [&] {
            std::uniform_int_distribution<int> big(0, 99);
            if (big(rng) < 95) {
                return rand_range(rng, args.small_payload);
            }
            return rand_range(rng, args.max_payload);
        };

        if (op == 0) {
            // insert
            size_t pos = biased_pos(rng, len, last_pos);
            size_t n = pick_payload_len();
            std::string txt = rand_text(rng, n);

            checkpoint_for_edit();
            pt.insert(pos, txt);
            model.insert(pos, txt);

            last_pos = pos;
            push_log("insert pos=" + std::to_string(pos) + " n=" + std::to_string(n));
        } else if (op == 1) {
            // erase
            if (len == 0) {
                push_log("erase (skipped: empty)");
            } else {
                size_t pos = biased_pos(rng, len - 1, last_pos);
                size_t max_del = len - pos;
                size_t count = rand_range(rng, std::min<size_t>(max_del, 2048));  // keep it fast

                checkpoint_for_edit();
                pt.erase(pos, count);
                model.erase(pos, count);

                last_pos = pos;
                push_log("erase pos=" + std::to_string(pos) + " count=" + std::to_string(count));
            }
        } else if (op == 2) {
            // replace == erase + insert at same position
            size_t pos = (len == 0) ? 0 : biased_pos(rng, len - 1, last_pos);
            size_t max_del = (len >= pos) ? (len - pos) : 0;
            size_t del = rand_range(rng, std::min<size_t>(max_del, 1024));
            size_t n = pick_payload_len();
            std::string txt = rand_text(rng, n);

            checkpoint_for_edit();
            pt.erase(pos, del);
            pt.insert(pos, txt);

            model.erase(pos, del);
            model.insert(pos, txt);

            last_pos = pos;
            push_log("replace pos=" + std::to_string(pos) + " del=" + std::to_string(del) +
                     " ins=" + std::to_string(n));
        } else if (op == 3) {
            // undo
            bool ok_pt = pt.undo();
            bool ok_model = !undo_stack.empty();
            if (ok_model) {
                redo_stack.push_back(model);
                model = undo_stack.back();
                undo_stack.pop_back();
            }
            if (ok_pt != ok_model) {
                fail(args, seed, step, "undo mismatch (pt vs model)", pt, model, log);
            }
            push_log("undo");
        } else if (op == 4) {
            // redo
            bool ok_pt = pt.redo();
            bool ok_model = !redo_stack.empty();
            if (ok_model) {
                undo_stack.push_back(model);
                model = redo_stack.back();
                redo_stack.pop_back();
            }
            if (ok_pt != ok_model) {
                fail(args, seed, step, "redo mismatch (pt vs model)", pt, model, log);
            }
            push_log("redo");
        } else if (op == 6) {
            // clear
            checkpoint_for_edit();
            pt.clear();
            model.clear();
            last_pos = 0;
            push_log("clear");
        } else {
            // queries-only (no state change)
            push_log("query");
        }

        // Cheap invariants always
        if (pt.length() != model.size()) {
            fail(args, seed, step, "length mismatch", pt, model, log);
        }
        // Line count sanity (cheap-ish)
        {
            size_t lf = std::ranges::count(model, '\n');
            if (pt.line_feed_count() != lf) {
                fail(args, seed, step, "line_feed_count mismatch", pt, model, log);
            }
            if (pt.line_count() != lf + 1) {
                fail(args, seed, step, "line_count mismatch", pt, model, log);
            }
        }

        // Random query consistency checks
        {
            // line_at vs model_line_at
            size_t off = rand_range(rng, model.size());
            size_t la_pt = pt.line_at(off);
            size_t la_m = model_line_at(model, off);
            if (la_pt != la_m) {
                fail(args, seed, step, "line_at mismatch", pt, model, log);
            }

            // offset_at round-trip
            size_t line = (pt.line_count() == 0) ? 0 : rand_range(rng, pt.line_count() - 1);
            size_t col = rand_range(rng, 200);  // arbitrary
            size_t off_m = model_offset_at(model, line, col);
            size_t off_pt = pt.offset_at(line, col);
            if (off_pt != off_m) {
                fail(args, seed, step, "offset_at mismatch", pt, model, log);
            }

            // substr vs model substring (bounded)
            if (!model.empty()) {
                size_t pos = rand_range(rng, model.size() - 1);
                size_t maxn = model.size() - pos;
                size_t n = rand_range(rng, std::min(maxn, size_t{256}));
                std::string s_pt = pt.substr(pos, n);
                std::string s_m = model.substr(pos, n);
                if (s_pt != s_m) {
                    fail(args, seed, step, "substr mismatch", pt, model, log);
                }
            }
        }

        // Expensive full-content check occasionally
        if (args.check_every != 0 && (step % args.check_every) == 0) {
            std::string s = pt.str();
            if (s != model) {
                fail(args, seed, step, "pt.str() mismatch", pt, model, log);
            }
        }
    }

    spdlog::info("OK ({} ops), seed={}", args.ops, seed);
    return 0;
}
