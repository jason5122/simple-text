#include "base/hash/hash.h"
#include "build/build_config.h"
#include <gtest/gtest.h>

namespace base {

TEST(HashTest, StringDeterministic) {
    EXPECT_EQ(hash_string(""), hash_string(""));
    EXPECT_EQ(hash_string("abc"), hash_string("abc"));
    // Embedded NUL shouldn’t truncate
    std::string s("a\0b", 3);
    EXPECT_EQ(hash_string(s), hash_string(s));
}

#if BUILDFLAG(IS_WIN)
TEST(HashTest, WStringBytesMatch) {
    // Only asserts our wstring overload hashes raw bytes as documented.
    std::wstring w = L"A\u00E9\u6C34";  // depends on platform width/endianness
    const auto* bytes = reinterpret_cast<const char*>(w.data());
    const size_t len_bytes = w.size() * sizeof(wchar_t);
    EXPECT_EQ(hash_string(w), rapidhash(bytes, len_bytes));
}
#endif

TEST(HashTest, CombineOrderMattersUsually) {
    // Not a strict guarantee, but a good smoke check that it's not symmetric.
    size_t a = hash_string("a");
    size_t b = hash_string("b");
    EXPECT_NE(hash_combine(a, b), hash_combine(b, a));
}

TEST(HashTest, CombineChangesWithEitherInput) {
    size_t seed = 0x12345678u;
    size_t h1 = hash_combine(seed, hash_string("x"));
    size_t h2 = hash_combine(seed, hash_string("y"));
    EXPECT_NE(h1, h2);
}

}  // namespace base
