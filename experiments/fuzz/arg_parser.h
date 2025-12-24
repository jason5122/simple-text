#pragma once

#include <cstdint>
#include <cstdlib>

struct Args {
    uint64_t seed = 0;
    bool seed_provided = false;
    size_t ops = 50'000;
    size_t max_payload = 4096;  // occasional larger inserts
    size_t small_payload = 32;  // typical insert size
    size_t check_every = 64;    // expensive full-string checks cadence
    size_t log_keep = 200;      // keep last N ops for debugging
};

uint64_t parse_u64(const char* s);
size_t parse_sz(const char* s);
Args parse_args(int argc, char** argv);
