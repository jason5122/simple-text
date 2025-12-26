#pragma once

#include <cstdint>
#include <cstdlib>

struct Args {
    size_t ops = 1000;
    size_t payload = 32;
    size_t check_every = 64;
};

uint64_t parse_u64(const char* s);
size_t parse_sz(const char* s);
Args parse_args(int argc, char** argv);
