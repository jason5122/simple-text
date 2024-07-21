#include "base/buffer/piece_table.h"
#include "base/numeric/saturation_arithmetic.h"
#include "gtest/gtest.h"
#include <random>

namespace {
std::mt19937 rng{std::random_device{}()};

// Returns a random number in the range [low, high].
inline int RandomNumber(int low, int high) {
    std::uniform_int_distribution<> distr{low, high};
    return distr(rng);
}

// Returns a random alphanumeric string of a specified length.
inline std::string RandomString(size_t length) {
    static constexpr std::string_view charset = "0123456789"
                                                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                "abcdefghijklmnopqrstuvwxyz";
    auto random_char = []() -> char { return charset.at(RandomNumber(0, charset.length() - 1)); };
    std::string str(length, 0);
    std::generate_n(str.begin(), length, random_char);
    return str;
}

// Like `RandomString()`, but the string is guaranteed to contain a specified number of newlines.
inline std::string RandomNewlineString(size_t length, size_t newlines) {
    std::string str = RandomString(base::sub_sat(length, newlines));
    for (size_t k = 0; k < newlines; k++) {
        size_t i = RandomNumber(0, str.length());
        str.insert(i, "\n");
    }
    return str;
}

inline void CheckNewlineOffsets(std::string_view str, base::PieceTable& table) {
    EXPECT_EQ(table.newlineCount(), std::ranges::count(str, '\n'));

    size_t str_pos = str.find('\n');
    size_t line_index = 0;
    while (str_pos != std::string::npos) {
        size_t table_pos = std::distance(table.begin(), table.newline(line_index));
        EXPECT_EQ(str_pos, table_pos);

        str_pos = str.find('\n', str_pos + 1);
        ++line_index;
    }
    EXPECT_EQ(table.newline(table.newlineCount()), table.end());
}
}

namespace base {

TEST(PieceTableTest, Init) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

TEST(PieceTableTest, InsertAtBeginningOfString1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    const std::string s1 = "String1 ";
    str.insert(0, s1);
    table.insert(0, s1);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.insert(0, s1);
    table.insert(0, s1);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

TEST(PieceTableTest, InsertAtBeginningOfString2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    const std::string s1 = "String1 ";
    for (size_t n = 0; n < 5; n++) {
        str.insert(0, s1);
        table.insert(0, s1);
        EXPECT_EQ(str, table.str());
        EXPECT_EQ(str.length(), table.length());
    }
}

TEST(PieceTableTest, InsertAtBeginningOfPiece1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    const std::string s1 = "String1 ";
    str.insert(0, s1);
    table.insert(0, s1);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    const std::string s2 = "String2 ";
    str.insert(s1.length(), s2);
    table.insert(s1.length(), s2);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

TEST(PieceTableTest, InsertAtBeginningOfPiece2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    const std::string s1 = "String1 ";
    str.insert(0, s1);
    table.insert(0, s1);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    const std::string s2 = "String2 ";
    str.insert(s1.length(), s2);
    table.insert(s1.length(), s2);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    const std::string s3 = "String3 ";
    str.insert(s1.length(), s3);
    table.insert(s1.length(), s3);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    const std::string s4 = "String4 ";
    str.insert(s1.length() + s2.length(), s4);
    table.insert(s1.length() + s2.length(), s4);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

TEST(PieceTableTest, InsertAtMiddleOfPiece1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    const std::string s1 = "went to the park and\n";
    str.insert(20, s1);
    table.insert(20, s1);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    const std::string s2 = " and nimble";
    str.insert(9, s2);
    table.insert(9, s2);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

TEST(PieceTableTest, InsertAtMiddleOfPiece2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    const std::string s1 = "went to the park and\n";
    str.insert(20, s1);
    table.insert(20, s1);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    const std::string s2 = " and nimble";
    str.insert(9, s2);
    table.insert(9, s2);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    const std::string s3 = " sneaky,";
    str.insert(3, s3);
    table.insert(3, s3);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    const std::string s4 = "String4";
    str.insert(30, s4);
    table.insert(30, s4);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    const std::string s5 = "String5";
    str.insert(15, s5);
    table.insert(15, s5);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

TEST(PieceTableTest, InsertAtEnd1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    const std::string s1 = " and walked away\n";
    size_t len = str.length();
    str.insert(len, s1);
    table.insert(len, s1);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

TEST(PieceTableTest, InsertAtEnd2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    const std::string s1 = " and walked away\n";
    size_t len1 = str.length();
    str.insert(len1, s1);
    table.insert(len1, s1);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    const std::string s2 = "went to the park and\n";
    str.insert(20, s2);
    table.insert(20, s2);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    const std::string s3 = " and nimble";
    str.insert(9, s3);
    table.insert(9, s3);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    const std::string s4 = " from the dog\n";
    size_t len2 = str.length();
    str.insert(len2, s4);
    table.insert(len2, s4);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

TEST(PieceTableTest, InsertCustomTest1) {
    std::string str = "";
    PieceTable table{str};

    const std::string s1 = "R7";
    str.insert(0, s1);
    table.insert(0, s1);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    const std::string s2 = "nB";
    str.insert(1, s2);
    table.insert(1, s2);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    const std::string s3 = "6D";
    str.insert(2, s3);
    table.insert(2, s3);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

TEST(PieceTableTest, InsertEmpty) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    str.insert(0, "");
    table.insert(0, "");
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.insert(10, "");
    table.insert(10, "");
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.insert(18, "");
    table.insert(18, "");
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.insert(str.length(), "");
    table.insert(str.length(), "");
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

// Randomly inserts 100 alphanumeric strings of length [0, 10] at index [0, length).
TEST(PieceTableTest, InsertAtRandom) {
    std::string str = "";
    PieceTable table{str};

    for (size_t n = 0; n < 100; n++) {
        size_t index = RandomNumber(0, str.length());
        const std::string random_str = RandomString(RandomNumber(0, 10));

        str.insert(index, random_str);
        table.insert(index, random_str);
        EXPECT_EQ(str, table.str());
        EXPECT_EQ(str.length(), table.length());
    }
}

TEST(PieceTableTest, EraseAtBeginning1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    while (str.length() > 0) {
        str.erase(0, 1);
        table.erase(0, 1);
        EXPECT_EQ(str, table.str());
        EXPECT_EQ(str.length(), table.length());
    }
}

TEST(PieceTableTest, EraseAtMiddleOfPiece1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    str.erase(3, 6);
    table.erase(3, 6);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.erase(4, 10);
    table.erase(4, 10);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

TEST(PieceTableTest, EraseAtMiddleOfPiece2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    str.erase(3, 6);
    table.erase(3, 6);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.erase(4, 10);
    table.erase(4, 10);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.erase(4, 21);
    table.erase(4, 21);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

TEST(PieceTableTest, EraseBeyondOnePiece1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    const std::string s1 = " and nimble";
    str.insert(9, s1);
    table.insert(9, s1);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.erase(4, 17);
    table.erase(4, 17);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

TEST(PieceTableTest, EraseBeyondOnePiece2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    const std::string s1 = "String1";
    str.insert(0, s1);
    table.insert(0, s1);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.erase(1, str.length());
    table.erase(1, table.length());
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

TEST(PieceTableTest, EraseBeyondOnePiece3) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    const std::string s1 = "String1";
    str.insert(10, s1);
    table.insert(10, s1);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.erase(0, 9999);
    table.erase(0, 9999);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

TEST(PieceTableTest, EraseCustomTest1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    str.erase(25, 44);
    table.erase(25, 44);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.erase(9, 14);
    table.erase(9, 14);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

TEST(PieceTableTest, EraseCustomTest2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    str.erase(3, 2);
    table.erase(3, 2);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.erase(7, 15);
    table.erase(7, 15);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.erase(4, 4);
    table.erase(4, 4);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.erase(22, 7);
    table.erase(22, 7);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

TEST(PieceTableTest, EraseCustomTest3) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    str.erase(1, 40);
    table.erase(1, 40);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.erase(1, 0);
    table.erase(1, 0);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.erase(1, 2);
    table.erase(1, 2);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.erase(2, 0);
    table.erase(2, 0);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

TEST(PieceTableTest, EraseEmpty) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    str.erase(1, 0);
    table.erase(1, 0);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.erase(8, 0);
    table.erase(8, 0);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.erase(0, 0);
    table.erase(0, 0);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.erase(str.length(), 0);
    table.erase(str.length(), 0);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

// Randomly erases 10 times from the string at index [0, length] and count [0, length).
// We repeat this for 100 iterations.
TEST(PieceTableTest, EraseAtRandom) {
    constexpr std::string_view original_str = "The quick brown fox\njumped over the lazy dog";

    for (size_t n = 0; n < 100; n++) {
        std::string str{original_str};
        PieceTable table{original_str};

        for (size_t i = 0; i < 10; i++) {
            size_t index = RandomNumber(0, str.length());
            size_t count = RandomNumber(0, str.length());

            str.erase(index, count);
            table.erase(index, count);
            EXPECT_EQ(str, table.str());
            EXPECT_EQ(str.length(), table.length());
        }
    }
}

TEST(PieceTableTest, CombinedRandomTest1) {
    std::string str = "";
    PieceTable table{str};

    for (size_t n = 0; n < 100; n++) {
        // Randomly insert.
        size_t insert_index = RandomNumber(0, str.length());
        const std::string random_str = RandomString(RandomNumber(0, 10));
        str.insert(insert_index, random_str);
        table.insert(insert_index, random_str);
        EXPECT_EQ(str, table.str());
        EXPECT_EQ(str.length(), table.length());

        // Randomly erase.
        size_t erase_index = RandomNumber(0, str.length());
        size_t count = RandomNumber(0, 4);
        str.erase(erase_index, count);
        table.erase(erase_index, count);
        EXPECT_EQ(str, table.str());
        EXPECT_EQ(str.length(), table.length());
    }
}

TEST(PieceTableTest, IteratorIncrementTest) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    table.insert(20, "went to the park and\n");
    table.insert(9, " and nimble");

    // Iterate manually.
    std::string temp1;
    auto it1 = table.begin();
    for (size_t i = 0; i < table.length(); i++) {
        EXPECT_NE(it1, table.end());
        temp1 += *it1;
        it1++;
    }
    EXPECT_EQ(it1, table.end());
    EXPECT_EQ(temp1, table.str());

    // Iterate using for loop.
    std::string temp2;
    for (auto it = table.begin(); it != table.end(); it++) {
        EXPECT_NE(it, table.end());
        temp2 += *it;
    }
    EXPECT_EQ(temp2, table.str());

    // Iterate using range-based for loop.
    std::string temp3;
    for (char ch : table) {
        temp3 += ch;
    }
    EXPECT_EQ(temp3, table.str());
}

TEST(PieceTableTest, IteratorForwardIteratorTest) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    // `std::fill` accepts a forward iterator.
    std::fill(table.begin(), table.end(), 'e');

    for (char ch : table) {
        EXPECT_EQ(ch, 'e');
    }
}

TEST(PieceTableTest, IteratorLineTest1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    CheckNewlineOffsets(str, table);
}

TEST(PieceTableTest, IteratorLineTest2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    const std::string s1 = "String1 ";
    str.insert(0, s1);
    table.insert(0, s1);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    CheckNewlineOffsets(str, table);
}

TEST(PieceTableTest, IteratorLineTest3) {
    std::string str = "The \nquick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    const std::string s1 = "String1 ";
    str.insert(0, s1);
    table.insert(0, s1);
    table.insert(13, "");  // This should split up a piece internally.

    CheckNewlineOffsets(str, table);
}

TEST(PieceTableTest, IteratorNewlineInsertTest1) {
    std::string str = "The \nquick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    const std::string str_with_newline = "hello\n";
    str.insert(0, str_with_newline);
    table.insert(0, str_with_newline);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.insert(5, str_with_newline);
    table.insert(5, str_with_newline);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.insert(str.length(), str_with_newline);
    table.insert(table.length(), str_with_newline);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.insert(20, str_with_newline);
    table.insert(20, str_with_newline);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.insert(10, str_with_newline);
    table.insert(10, str_with_newline);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    EXPECT_EQ(table.newlineCount(), std::ranges::count(str, '\n'));

    auto str_it = str.begin();
    auto table_it = table.begin();
    while (str_it != str.end() && table_it != table.end()) {
        if ((*str_it) == '\n') {
            EXPECT_EQ(*table_it, '\n');
        } else {
            EXPECT_NE(*table_it, '\n');
        }

        str_it++;
        table_it++;
    }
}

TEST(PieceTableTest, IteratorRandomNewlineInsertTest1) {
    std::string str = "The \nquick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    for (size_t n = 0; n < 100; n++) {
        size_t index = RandomNumber(0, str.length());
        const std::string newline_str = RandomNewlineString(RandomNumber(1, 10), 1);

        str.insert(index, newline_str);
        table.insert(index, newline_str);
        EXPECT_EQ(str, table.str());
        EXPECT_EQ(str.length(), table.length());
    }

    CheckNewlineOffsets(str, table);
}

TEST(PieceTableTest, IteratorRandomNewlineInsertTest2) {
    std::string str = "The \nquick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    for (size_t n = 0; n < 100; n++) {
        size_t index = RandomNumber(0, str.length());
        const std::string newline_str = RandomNewlineString(RandomNumber(1, 50), 5);

        str.insert(index, newline_str);
        table.insert(index, newline_str);
        EXPECT_EQ(str, table.str());
        EXPECT_EQ(str.length(), table.length());
    }

    CheckNewlineOffsets(str, table);
}

TEST(PieceTableTest, IteratorNewlineEraseTest1) {
    std::string str = "\nThe \nquick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    str.erase(11, 6);
    table.erase(11, 6);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    CheckNewlineOffsets(str, table);
}

TEST(PieceTableTest, IteratorNewlineEraseTest2) {
    std::string str = "\nThe \nquick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    str.erase(5, 20);
    table.erase(5, 20);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    CheckNewlineOffsets(str, table);
}

TEST(PieceTableTest, IteratorNewlineEraseTest3) {
    std::string str = "\nThe \nquick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    const std::string s1 = "adept\n";
    str.insert(10, s1);
    table.insert(10, s1);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.erase(5, 22);
    table.erase(5, 22);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    CheckNewlineOffsets(str, table);
}

TEST(PieceTableTest, IteratorRandomNewlineEraseTest1) {
    for (size_t n = 0; n < 100; n++) {
        std::string str = RandomNewlineString(100, 20);
        PieceTable table{str};

        for (size_t i = 0; i < 10; i++) {
            size_t index = RandomNumber(0, str.length());
            size_t count = RandomNumber(0, 5);

            str.erase(index, count);
            table.erase(index, count);
            EXPECT_EQ(str, table.str());
            EXPECT_EQ(str.length(), table.length());
        }

        CheckNewlineOffsets(str, table);
    }
}

TEST(PieceTableTest, IteratorCustomNewlineEraseTest1) {
    std::string str = "\n";
    PieceTable table{str};

    str.erase(0, 0);
    table.erase(0, 0);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.erase(0, 0);
    table.erase(0, 0);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    str.erase(1, 0);
    table.erase(1, 0);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());

    CheckNewlineOffsets(str, table);
}

TEST(PieceTableTest, IteratorCustomNewlineEraseTest2) {
    std::string str = "";
    PieceTable table{str};

    for (size_t k = 0; k < 6; k++) {
        str.erase(0, 0);
        table.erase(0, 0);
        EXPECT_EQ(str, table.str());
        EXPECT_EQ(str.length(), table.length());
    }

    CheckNewlineOffsets(str, table);
}

TEST(PieceTableTest, IteratorEmptyTest1) {
    PieceTable table{""};
    EXPECT_EQ(table.begin(), table.end());
}

TEST(PieceTableTest, IteratorEmptyTest2) {
    PieceTable table{"hi"};
    EXPECT_NE(table.begin(), table.end());

    table.erase(0, 100);
    EXPECT_EQ(table.begin(), table.end());

    table.insert(0, "hello");
    EXPECT_NE(table.begin(), table.end());

    table.erase(0, 4);
    EXPECT_NE(table.begin(), table.end());

    table.erase(0, 1);
    EXPECT_EQ(table.begin(), table.end());
}

TEST(PieceTableTest, IteratorLineContentTest1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable table{str};

    std::string line0;
    for (auto it = table.begin(); it != table.newline(0); it++) {
        line0 += *it;
    }

    std::string line1;
    for (auto it = std::next(table.newline(0)); it != table.newline(1); it++) {
        line1 += *it;
    }

    size_t str_pos = str.find('\n');
    EXPECT_EQ(line0, str.substr(0, str_pos));
    EXPECT_EQ(line1, str.substr(str_pos + 1));
}

}
