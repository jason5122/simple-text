#pragma once

#include <span>
#include <vector>

namespace base {

// Returns a random number in range [0, UINT64_MAX]. Thread-safe.
uint64_t rand_uint64();

// Returns a random number in range [min, max]. Thread-safe.
int rand_int(int min, int max);

// Returns a random double in range [0, 1). Thread-safe.
double rand_double();

// Returns a random float in range [0, 1). Thread-safe.
float rand_float();

// Returns a random number in range [0, range).  Thread-safe.
uint64_t rand_generator(uint64_t range);

// Fills `output` with cryptographically secure random data. Thread-safe.
void rand_bytes(std::span<uint8_t> output);

// Fills a string of length |length| with random data and returns it. Thread-safe.
std::string rand_bytes_as_string(size_t length);

// Creates a vector of `length` bytes, fills it with random data, and returns it. Thread-safe.
std::vector<uint8_t> rand_bytes_as_vector(size_t length);

// Generates a lowercase alphabetical string (a-z) with k newlines.
std::string rand_string_with_newlines(size_t length, size_t k);

}  // namespace base
