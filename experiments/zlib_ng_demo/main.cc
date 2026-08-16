// Round-trips a payload through zlib-ng's classic (ZLIB_COMPAT) API to confirm
// the library actually *runs* on the host CPU, not just that it compiled. The
// deflate/inflate and crc32 calls dispatch through zlib-ng's function table, so
// this exercises whichever SIMD path (NEON/ARMv8-CRC, SSE2..AVX2/PCLMUL) the
// runtime CPU detection selected. Exits non-zero on any mismatch so it can be
// run as a per-platform smoke test in CI.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <print>
#include <span>
#include <vector>

#include "zlib.h"

namespace {

// A deterministic, moderately compressible 1 MiB payload: repeated runs give
// the match finder (slide_hash, compare256) real work, while an LCG varies the
// bytes so literal and chunk-copy paths are exercised too.
std::vector<unsigned char> MakePayload(std::size_t size) {
    std::vector<unsigned char> data;
    data.reserve(size + 16);
    std::uint32_t state = 0x12345678u;
    while (data.size() < size) {
        state = (state * 1664525u) + 1013904223u;
        auto byte = static_cast<unsigned char>(state >> 24);
        std::size_t run = 4 + (byte & 0x0fu);
        data.insert(data.end(), run, byte);
    }
    data.resize(size);
    return data;
}

std::uint32_t Crc32(std::span<const unsigned char> data) {
    uLong crc = crc32(0, nullptr, 0);
    crc = crc32(crc, data.data(), static_cast<uInt>(data.size()));
    return static_cast<std::uint32_t>(crc);
}

}  // namespace

int main() {
    std::println("zlib-ng version: {}", zlibVersion());

    const std::vector<unsigned char> input = MakePayload(1u << 20);
    const std::uint32_t input_crc = Crc32(input);

    std::vector<unsigned char> compressed(compressBound(static_cast<uLong>(input.size())));
    uLongf compressed_len = static_cast<uLongf>(compressed.size());
    int rc = compress2(compressed.data(), &compressed_len, input.data(),
                       static_cast<uLong>(input.size()), Z_BEST_COMPRESSION);
    if (rc != Z_OK) {
        std::println("compress2 failed: {}", rc);
        return 1;
    }
    compressed.resize(compressed_len);

    std::vector<unsigned char> output(input.size());
    uLongf output_len = static_cast<uLongf>(output.size());
    rc = uncompress(output.data(), &output_len, compressed.data(),
                    static_cast<uLong>(compressed.size()));
    if (rc != Z_OK) {
        std::println("uncompress failed: {}", rc);
        return 1;
    }
    output.resize(output_len);

    const bool ok = std::ranges::equal(input, output) && Crc32(output) == input_crc;

    std::println("input:      {} bytes  crc32=0x{:08x}", input.size(), input_crc);
    std::println("compressed: {} bytes  ({:.1f}% of input)", compressed.size(),
                 100.0 * static_cast<double>(compressed.size()) /
                     static_cast<double>(input.size()));
    std::println("round-trip: {}", ok ? "OK" : "MISMATCH");
    return ok ? 0 : 1;
}
