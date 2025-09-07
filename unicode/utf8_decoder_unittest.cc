#include "unicode/utf8_decoder.h"
#include <gtest/gtest.h>

namespace unicode {

TEST(UTF8DecoderTest, Valid) {
    UTF8Decoder decoder;
    decoder.put(0x68);
    EXPECT_TRUE(decoder.done());
    EXPECT_FALSE(decoder.error());
    EXPECT_EQ(decoder.value(), 0x68);
}

}  // namespace unicode
