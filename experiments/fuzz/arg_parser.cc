#include "experiments/fuzz/arg_parser.h"
#include <spdlog/spdlog.h>
#include <string_view>

uint64_t parse_u64(const char* s) {
    char* end = nullptr;
    unsigned long long v = std::strtoull(s, &end, 10);
    if (!s[0] || (end && *end)) {
        spdlog::error("Bad integer: {}", s);
        std::exit(2);
    }
    return static_cast<uint64_t>(v);
}

size_t parse_sz(const char* s) { return static_cast<size_t>(parse_u64(s)); }

Args parse_args(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; i++) {
        std::string_view k = argv[i];
        auto need = [&](const char* name) -> const char* {
            if (i + 1 >= argc) {
                spdlog::error("Missing value for {}", name);
                std::exit(2);
            }
            return argv[++i];
        };

        if (k == "--seed") {
            a.seed = parse_u64(need("--seed"));
            a.seed_provided = true;
        } else if (k == "--ops") {
            a.ops = parse_sz(need("--ops"));
        } else if (k == "--max_payload") {
            a.max_payload = parse_sz(need("--max_payload"));
        } else if (k == "--check_every") {
            a.check_every = parse_sz(need("--check_every"));
        } else if (k == "--log_keep") {
            a.log_keep = parse_sz(need("--log_keep"));
        } else {
            spdlog::error("Unknown arg: {}", k);
            std::exit(2);
        }
    }
    return a;
}
