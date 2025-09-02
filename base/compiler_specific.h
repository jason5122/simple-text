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
