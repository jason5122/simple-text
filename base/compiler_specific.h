#pragma once

#if defined(__has_builtin)
#define HAS_BUILTIN(x) __has_builtin(x)
#else
#define HAS_BUILTIN(x) 0
#endif

#if __has_cpp_attribute(clang::always_inline)
#define ALWAYS_INLINE [[clang::always_inline]] inline
#elif __has_cpp_attribute(gnu::always_inline)
#define ALWAYS_INLINE [[gnu::always_inline]] inline
#elif defined(COMPILER_MSVC)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline
#endif

#if __has_cpp_attribute(gnu::noinline)
#define NOINLINE [[gnu::noinline]]
#elif __has_cpp_attribute(msvc::noinline)
#define NOINLINE [[msvc::noinline]]
#else
#define NOINLINE
#endif

// https://devblogs.microsoft.com/cppblog/msvc-cpp20-and-the-std-cpp20-switch/#msvc-extensions-and-abi
#if __has_cpp_attribute(msvc::no_unique_address)
#define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#elif __has_cpp_attribute(no_unique_address)
#define NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
#define NO_UNIQUE_ADDRESS
#endif

// Annotates code indicating that it should be permanently exempted from
// `-Wunsafe-buffer-usage`. For temporary cases such as migrating callers to
// safer patterns, use `UNSAFE_TODO()` instead; see documentation there.
//
// ** USE OF THIS MACRO SHOULD BE VERY RARE.** Using this macro indicates that
// the compiler cannot verify that the code avoids OOB, and manual review is
// required. Even with manual review, it's easy for assumptions to change and
// security bugs to creep in over time. Prefer safer patterns instead.
//
// Disabling `clang-format` allows each `_Pragma` to be on its own line, as
// recommended by https://gcc.gnu.org/onlinedocs/cpp/Pragmas.html.
// clang-format off
#define UNSAFE_BUFFERS(...)                  \
  _Pragma("clang unsafe_buffer_usage begin") \
  __VA_ARGS__                                \
  _Pragma("clang unsafe_buffer_usage end")
// clang-format on

// Annotates code indicating that it should be temporarily exempted from
// `-Wunsafe-buffer-usage`. While this is functionally the same as
// `UNSAFE_BUFFERS()`, semantically it indicates that this is for migration
// purposes, and should be cleaned up as soon as possible.
#define UNSAFE_TODO(...) UNSAFE_BUFFERS(__VA_ARGS__)
