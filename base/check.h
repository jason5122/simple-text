#pragma once

#include "base/compiler_specific.h"
#include "base/location.h"
#include <cstdlib>

#define CHECK(cond)                                                                               \
    do {                                                                                          \
        if (!(cond)) [[unlikely]] {                                                               \
            ::base::internal::check_fail(#cond, ::base::Location::current());                     \
        }                                                                                         \
    } while (false)

#define CHECK_OP(op, a, b)                                                                        \
    do {                                                                                          \
        if (!((a)op(b))) [[unlikely]] {                                                           \
            ::base::internal::check_op_fail(#op, #a, #b, ::base::Location::current());            \
        }                                                                                         \
    } while (false)

#define CHECK_EQ(a, b) CHECK_OP(==, a, b)
#define CHECK_NE(a, b) CHECK_OP(!=, a, b)
#define CHECK_LT(a, b) CHECK_OP(<, a, b)
#define CHECK_LE(a, b) CHECK_OP(<=, a, b)
#define CHECK_GT(a, b) CHECK_OP(>, a, b)
#define CHECK_GE(a, b) CHECK_OP(>=, a, b)

#define NOTREACHED()                                                                              \
    do {                                                                                          \
        ::base::internal::notreached_fail(::base::Location::current());                           \
    } while (false)

#ifdef NDEBUG
// clang-format off
#define DCHECK(cond)        do { (void)sizeof(cond); } while (false)
#define DCHECK_EQ(a, b)     do { (void)sizeof((a) == (b)); } while (false)
#define DCHECK_NE(a, b)     do { (void)sizeof((a) != (b)); } while (false)
#define DCHECK_LT(a, b)     do { (void)sizeof((a)  < (b)); } while (false)
#define DCHECK_LE(a, b)     do { (void)sizeof((a) <= (b)); } while (false)
#define DCHECK_GT(a, b)     do { (void)sizeof((a)  > (b)); } while (false)
#define DCHECK_GE(a, b)     do { (void)sizeof((a) >= (b)); } while (false)
// clang-format on
#else
#define DCHECK(cond) CHECK(cond)
#define DCHECK_EQ(a, b) CHECK_EQ(a, b)
#define DCHECK_NE(a, b) CHECK_NE(a, b)
#define DCHECK_LT(a, b) CHECK_LT(a, b)
#define DCHECK_LE(a, b) CHECK_LE(a, b)
#define DCHECK_GT(a, b) CHECK_GT(a, b)
#define DCHECK_GE(a, b) CHECK_GE(a, b)
#endif

namespace base::internal {

[[noreturn]] NOINLINE void check_fail(const char* expr, Location loc);
[[noreturn]] NOINLINE void check_op_fail(const char* op,
                                         const char* a,
                                         const char* b,
                                         Location loc);
[[noreturn]] NOINLINE void notreached_fail(Location loc);

}  // namespace base::internal
