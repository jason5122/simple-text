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

// Annotates a data member indicating it need not have an address distinct from all other
// non-static data members of the class, and its tail padding may be used for other objects'
// storage. This can have subtle and dangerous effects, including on containing objects; use with
// caution.
//
// See also:
//   https://en.cppreference.com/w/cpp/language/attributes/no_unique_address
//   https://wg21.link/dcl.attr.nouniqueaddr
// Usage:
// ```
//   // In the following struct, `t` might not have a unique address from `i`,
//   // and `t`'s tail padding (if any) may be reused by subsequent objects.
//   struct S {
//     int i;
//     NO_UNIQUE_ADDRESS T t;
//   };
// ```
//
// Unfortunately MSVC ignores [[no_unique_address]] (see
// https://devblogs.microsoft.com/cppblog/msvc-cpp20-and-the-std-cpp20-switch/#msvc-extensions-and-abi),
// and clang-cl matches it for ABI compatibility reasons. We need to prefer
// [[msvc::no_unique_address]] when available if we actually want any effect.
#if __has_cpp_attribute(msvc::no_unique_address)
#define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#elif __has_cpp_attribute(no_unique_address)
#define NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
#define NO_UNIQUE_ADDRESS
#endif
