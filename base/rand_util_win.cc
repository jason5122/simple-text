#include "base/check.h"
#include "base/rand_util.h"
#include <windows.h>

// Prototype for ProcessPrng.
// See: https://learn.microsoft.com/en-us/windows/win32/seccng/processprng
extern "C" {
BOOL WINAPI ProcessPrng(PBYTE pbData, SIZE_T cbData);
}

namespace base {

namespace {

// Import bcryptprimitives!ProcessPrng rather than cryptbase!RtlGenRandom to
// avoid opening a handle to \\Device\KsecDD in the renderer.
decltype(&ProcessPrng) GetProcessPrng() {
    HMODULE hmod = LoadLibraryW(L"bcryptprimitives.dll");
    CHECK(hmod);
    decltype(&ProcessPrng) process_prng_fn =
        reinterpret_cast<decltype(&ProcessPrng)>(GetProcAddress(hmod, "ProcessPrng"));
    CHECK(process_prng_fn);
    return process_prng_fn;
}

}  // namespace

void rand_bytes(std::span<uint8_t> output) {
    static decltype(&ProcessPrng) process_prng_fn = GetProcessPrng();
    BOOL success = process_prng_fn(static_cast<BYTE*>(output.data()), output.size());
    // ProcessPrng is documented to always return TRUE.
    CHECK(success);
}

}  // namespace base
