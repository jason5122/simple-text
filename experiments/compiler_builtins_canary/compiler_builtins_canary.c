// //experiments/compiler_builtins_canary.c
//
// Build canary: forces calls into clang_rt.builtins across several builtin
// families. If the compiler-rt builtins runtime is not linked, this fails to
// link with undefined __*ti3 / __float* / __fix* / __mul*c3 / __div*c3 symbols.
//
// NOTE: This reliably *tests* the builtins link only where the driver does NOT
// auto-link compiler-rt — i.e. Windows/clang-cl. On Mac/Linux (gcc-style
// driver) compiler-rt is auto-linked, so this compiles even absent our GN
// config; there it is a secondary safety net, not a config test.
//
// Each helper is noinline and operands/results are volatile to prevent the
// optimizer from constant-folding or inlining the operation away, which would
// silently turn this canary into a no-op.

#include <stdint.h>

// --- 128-bit integer division/modulo: __udivti3 / __umodti3 ---
// No hardware 128-bit divide, so these reliably lower to runtime calls.
__attribute__((noinline)) unsigned __int128 u128_divmod(unsigned __int128 a, unsigned __int128 b) {
    return (a / b) + (a % b);
}

// --- Signed 128-bit division/modulo: __divti3 / __modti3 ---
__attribute__((noinline)) __int128 s128_divmod(__int128 a, __int128 b) {
    return (a / b) + (a % b);
}

// --- 128-bit multiply: __multi3 ---
// Often partially inlined, but full 128x128 with noinline+volatile usually
// still emits __multi3. Kept for coverage; not the primary guarantee.
__attribute__((noinline)) unsigned __int128 u128_mul(unsigned __int128 a, unsigned __int128 b) {
    return a * b;
}

// --- int <-> 128-bit / float conversions: __floatuntidf, __fixunsdfti, etc. ---
// Converting between 128-bit ints and floating point pulls in conversion
// helpers that have no single instruction.
__attribute__((noinline)) double u128_to_double(unsigned __int128 a) {
    return (double)a;  // __floatuntidf
}

__attribute__((noinline)) unsigned __int128 double_to_u128(double d) {
    return (unsigned __int128)d;  // __fixunsdfti
}

// --- Complex float/double multiply & divide: __mulsc3/__divsc3/__muldc3/__divdc3 ---
// These are the ones your libc++ shim also defines; exercising them here
// confirms *a* provider exists (shim or full runtime).
__attribute__((noinline)) _Complex float cf_muldiv(_Complex float a, _Complex float b) {
    return (a * b) + (a / b);  // __mulsc3, __divsc3
}

__attribute__((noinline)) _Complex double cd_muldiv(_Complex double a, _Complex double b) {
    return (a * b) + (a / b);  // __muldc3, __divdc3
}

int main(void) {
    // 128-bit divmod (the primary, reliably-firing check)
    volatile unsigned __int128 ua =
        (unsigned __int128)0x123456789abcdef0ULL << 64 | 0xfedcba9876543210ULL;
    volatile unsigned __int128 ub = 0xdeadbeefULL;
    volatile unsigned __int128 ur = u128_divmod(ua, ub);

    volatile __int128 sa = -(__int128)ua;
    volatile __int128 sb = 0x1234567ULL;
    volatile __int128 sr = s128_divmod(sa, sb);

    // 128-bit multiply
    volatile unsigned __int128 mr = u128_mul(ua, ub);

    // conversions
    volatile double df = u128_to_double(ua);
    volatile unsigned __int128 dr = double_to_u128(df);

    // complex arithmetic
    volatile _Complex float cf =
        cf_muldiv(1.5f + 2.5f * (_Complex float)1.0fi, 0.5f + 1.0f * (_Complex float)1.0fi);
    volatile _Complex double cd =
        cd_muldiv(1.5 + 2.5 * (_Complex double)1.0i, 0.5 + 1.0 * (_Complex double)1.0i);

    // Combine everything into the return value so nothing is dead-code
    // eliminated. The exact value doesn't matter; we only need the ops to
    // survive to link time.
    volatile int acc = 0;
    acc += (int)ur;
    acc += (int)sr;
    acc += (int)mr;
    acc += (int)dr;
    acc += (int)df;
    acc += (int)__real__ cf;
    acc += (int)__real__ cd;
    return acc & 0x7f;
}
