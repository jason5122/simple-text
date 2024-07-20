#include "base/buffer/piece_table.h"
#include "gtest/gtest.h"
#include <random>

namespace base {

TEST(PieceTableTest, Init) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    base::PieceTable table{str};
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

TEST(PieceTableTest, InsertAtBeginningOfString1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    base::PieceTable table{str};

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
    base::PieceTable table{str};

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
    base::PieceTable table{str};

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
    base::PieceTable table{str};

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
    base::PieceTable table{str};

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
    base::PieceTable table{str};

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
    base::PieceTable table{str};

    const std::string s1 = " and walked away\n";
    size_t len = str.length();
    str.insert(len, s1);
    table.insert(len, s1);
    EXPECT_EQ(str, table.str());
    EXPECT_EQ(str.length(), table.length());
}

TEST(PieceTableTest, InsertAtEnd2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    base::PieceTable table{str};

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
    base::PieceTable table{str};

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
    base::PieceTable table{str};

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

static std::string RandomString(size_t length) {
    std::random_device rd;
    std::mt19937 gen{rd()};

    auto random_char = [&gen]() -> char {
        srand(time(nullptr));
        static constexpr std::string_view charset = "0123456789"
                                                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                    "abcdefghijklmnopqrstuvwxyz";
        std::uniform_int_distribution<> distr{0, charset.length() - 1};
        size_t i = distr(gen);
        return charset.at(i);
    };
    std::string str(length, 0);
    std::generate_n(str.begin(), length, random_char);
    return str;
}

// Randomly inserts 100 alphanumeric strings of length [0, 10] at index [0, length).
TEST(PieceTableTest, InsertAtRandom) {
    std::string str = "";
    base::PieceTable table{str};

    std::random_device rd;
    std::mt19937 gen{rd()};

    constexpr int max_string_len = 10;
    std::uniform_int_distribution<> len_distr{0, max_string_len};

    for (size_t n = 0; n < 100; n++) {
        const int max_index = std::max(0, static_cast<int>(str.length()) - 1);
        std::uniform_int_distribution<> index_distr{0, max_index};

        size_t len = len_distr(gen);
        const std::string random_str = RandomString(len);

        size_t index = index_distr(gen);
        str.insert(index, random_str);
        table.insert(index, random_str);
        EXPECT_EQ(str, table.str());
        EXPECT_EQ(str.length(), table.length());
    }
}

TEST(PieceTableTest, EraseAtBeginning1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    base::PieceTable table{str};

    while (str.length() > 0) {
        str.erase(0, 1);
        table.erase(0, 1);
        EXPECT_EQ(str, table.str());
        EXPECT_EQ(str.length(), table.length());
    }
}

TEST(PieceTableTest, EraseAtMiddleOfPiece1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    base::PieceTable table{str};

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
    base::PieceTable table{str};

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
    base::PieceTable table{str};

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
    base::PieceTable table{str};

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
    base::PieceTable table{str};

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
    base::PieceTable table{str};

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
    base::PieceTable table{str};

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
    base::PieceTable table{str};

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
    base::PieceTable table{str};

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
    std::random_device rd;
    std::mt19937 gen{rd()};

    for (size_t n = 0; n < 100; n++) {
        std::string str{original_str};
        base::PieceTable table{original_str};

        for (size_t i = 0; i < 10; i++) {
            std::uniform_int_distribution<> index_distr{0, static_cast<int>(str.length())};
            std::uniform_int_distribution<> count_distr{0, static_cast<int>(str.length())};
            size_t index = index_distr(gen);
            size_t count = count_distr(gen);

            str.erase(index, count);
            table.erase(index, count);
            EXPECT_EQ(str, table.str());
            EXPECT_EQ(str.length(), table.length());
        }
    }
}

TEST(PieceTableTest, CombinedRandomTest1) {
    std::string str = "";
    base::PieceTable table{str};

    std::random_device rd;
    std::mt19937 gen{rd()};

    constexpr int max_string_len = 10;
    std::uniform_int_distribution<> len_distr{0, max_string_len};
    std::uniform_int_distribution<> count_distr{0, 4};

    for (size_t n = 0; n < 100; n++) {
        // Randomly insert.
        const int max_index = std::max(0, static_cast<int>(str.length()) - 1);
        std::uniform_int_distribution<> index_distr{0, max_index};

        size_t len = len_distr(gen);
        const std::string random_str = RandomString(len);

        size_t index = index_distr(gen);
        str.insert(index, random_str);
        table.insert(index, random_str);
        EXPECT_EQ(str, table.str());
        EXPECT_EQ(str.length(), table.length());

        // Randomly erase.
        index_distr = std::uniform_int_distribution<>{0, static_cast<int>(str.length())};
        index = index_distr(gen);
        size_t count = count_distr(gen);

        str.erase(index, count);
        table.erase(index, count);
        EXPECT_EQ(str, table.str());
        EXPECT_EQ(str.length(), table.length());
    }
}

TEST(PieceTableTest, IteratorIncrementTest) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    base::PieceTable table{str};

    const std::string s1 = "went to the park and\n";
    table.insert(20, s1);

    const std::string s2 = " and nimble";
    table.insert(9, s2);

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
    base::PieceTable table{str};

    // `std::fill` accepts a forward iterator.
    std::fill(table.begin(), table.end(), 'e');

    for (char ch : table) {
        EXPECT_EQ(ch, 'e');
    }
}

// TEST(PieceTableTest, LineTest1) {
//     std::string str = "The quick brown fox\njumped over the lazy dog";
//     base::PieceTable table{str};

//     for (size_t line_index = 0; line_index < table.lineCount(); line_index++) {
//         table.line(line_index);
//     }
// }

}
