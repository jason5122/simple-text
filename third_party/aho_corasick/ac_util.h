#pragma once

typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long uint64;
typedef unsigned char InputTy;

#define ASSERT(c) ((void)0)

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
