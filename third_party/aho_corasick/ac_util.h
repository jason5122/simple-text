#pragma once

#ifdef DEBUG
#include <stdio.h>   // for fprintf
#include <stdlib.h>  // for abort
#endif

typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long uint64;
typedef unsigned char InputTy;

#ifdef DEBUG
// Usage examples: ASSERT(a > b),  ASSERT(foo() && "Oops, foo() returned 0");
#define ASSERT(c)                                                                                 \
    if (!(c)) {                                                                                   \
        fprintf(stderr, "%s:%d Assert: %s\n", __FILE__, __LINE__, #c);                            \
        abort();                                                                                  \
    }
#else
#define ASSERT(c) ((void)0)
#endif

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#ifndef offsetof
#define offsetof(st, m) ((size_t)(&((st*)0)->m))
#endif

enum impl_var_t {
    IMPL_SLOW_VARIANT = 1,
    IMPL_FAST_VARIANT = 2,
};

#define AC_MAGIC_NUM 0x5a
struct buf_header_t {
    unsigned char magic_num;
    unsigned char impl_variant;
};
