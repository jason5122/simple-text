#include "base/buffer/piece_tree.h"
#include "third_party/uni_algo/include/uni_algo/prop.h"
#include <gtest/gtest.h>

// TODO: Debug use; remove this.
#include "util/std_print.h"

namespace base {

TEST(TreeWalkerTest, TreeWalkerNextUTF8Test1) {
    PieceTree tree{"abcdefghijklmnopqrstuvwxyz"};
    TreeWalker walker{&tree};

    while (!walker.exhausted()) {
        int32_t codepoint = walker.next_codepoint();
        EXPECT_TRUE(una::codepoint::is_alphabetic(codepoint));
    }
    EXPECT_EQ(walker.next_codepoint(), 0);
}

TEST(TreeWalkerTest, TreeWalkerNextUTF8Test2) {
    PieceTree tree1{"﷽"};
    TreeWalker walker1{&tree1};
    EXPECT_EQ(walker1.next_codepoint(), static_cast<int32_t>(U'\U0000FDFD'));
    EXPECT_TRUE(walker1.exhausted());

    PieceTree tree2{"☃️"};
    TreeWalker walker2{&tree2};
    EXPECT_EQ(walker2.next_codepoint(), static_cast<int32_t>(U'\U00002603'));
    EXPECT_FALSE(walker2.exhausted());
    EXPECT_EQ(walker2.next_codepoint(), static_cast<int32_t>(U'\U0000FE0F'));
    EXPECT_TRUE(walker2.exhausted());
}

// https://gershnik.github.io/2021/03/24/reverse-utf8-decoding.html
class ReverseUTF8Decoder {
public:
    constexpr void put(uint8_t byte) {
        uint32_t type = kStateTable[byte];
        m_state = kStateTable[256 + m_state + type];

        if (m_state <= kRejectState) {
            m_collect |= (((0xffu >> type) & (byte)) << m_shift);
            m_value = char32_t(m_collect);
            m_shift = 0;
            m_collect = 0;
        } else {
            m_collect |= ((byte & 0x3fu) << m_shift);
            m_shift += 6;
        }
    }

    constexpr bool done() const {
        return m_state == kAcceptState;
    }

    constexpr bool error() const {
        return m_state == kRejectState;
    }

    constexpr char32_t value() const {
        return m_value;
    }

private:
    static constexpr uint8_t kAcceptState = 0;
    static constexpr uint8_t kRejectState = 12;
    char32_t m_value = 0;
    uint32_t m_collect = 0;
    uint8_t m_shift = 0;
    uint8_t m_state = kAcceptState;

    // TODO: Format state table correctly.
    static constexpr const uint8_t kStateTable[] = {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  7,
        7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
        7,  7,  7,  7,  7,  7,  7,  7,  8,  8,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
        2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  10, 3,  3,  3,  3,  3,
        3,  3,  3,  3,  3,  3,  3,  4,  3,  3,  11, 6,  6,  6,  5,  8,  8,  8,  8,  8,  8,  8,  8,
        8,  8,  8,  0,  24, 12, 12, 12, 12, 12, 24, 12, 24, 12, 12, 0,  24, 12, 12, 12, 12, 12, 24,
        12, 24, 12, 12, 12, 36, 0,  12, 12, 12, 12, 48, 12, 36, 12, 12, 12, 60, 12, 0,  0,  12, 12,
        72, 12, 72, 12, 12, 12, 60, 12, 0,  12, 12, 12, 72, 12, 72, 0,  12, 12, 12, 12, 12, 12, 0,
        0,  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 0};
};

char32_t NextUTF8Backwards(ReverseTreeWalker& walker) {
    ReverseUTF8Decoder decoder;
    while (!walker.exhausted()) {
        decoder.put(walker.next());
        if (decoder.done()) {
            return decoder.value();
        } else if (decoder.error()) {
            std::println("invalid UTF-8");
            std::abort();
        }
    }
    if (!decoder.done()) {
        std::println("invalid UTF-8");
        std::abort();
    }
    return decoder.value();
}

TEST(TreeWalkerTest, ReverseTreeWalkerNextUTF8Test) {
    PieceTree tree{"a bc\u205Fxyz"};
    ReverseTreeWalker walker{&tree, tree.length()};
    EXPECT_EQ(NextUTF8Backwards(walker), U'z');
    EXPECT_EQ(NextUTF8Backwards(walker), U'y');
    EXPECT_EQ(NextUTF8Backwards(walker), U'x');
    EXPECT_EQ(NextUTF8Backwards(walker), U'\u205F');
    EXPECT_EQ(NextUTF8Backwards(walker), U'c');
    EXPECT_EQ(NextUTF8Backwards(walker), U'b');
    EXPECT_EQ(NextUTF8Backwards(walker), U' ');
    EXPECT_EQ(NextUTF8Backwards(walker), U'a');
    EXPECT_EQ(NextUTF8Backwards(walker), U'\0');
}

}  // namespace base
