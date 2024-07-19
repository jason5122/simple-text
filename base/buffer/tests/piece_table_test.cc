#include "base/buffer/piece_table.h"
#include "gtest/gtest.h"
#include <random>

namespace base {

TEST(PieceTableTest, Init) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    base::PieceTable table{str};
    EXPECT_EQ(str, table.str());
}

TEST(PieceTableTest, InsertAtBeginningOfString1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    base::PieceTable table{str};

    const std::string s1 = "String1 ";
    str.insert(0, s1);
    table.insert(0, s1);
    EXPECT_EQ(str, table.str());

    str.insert(0, s1);
    table.insert(0, s1);
    EXPECT_EQ(str, table.str());
}

TEST(PieceTableTest, InsertAtBeginningOfString2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    base::PieceTable table{str};

    const std::string s1 = "String1 ";
    for (size_t i = 0; i < 5; i++) {
        str.insert(0, s1);
        table.insert(0, s1);
        EXPECT_EQ(str, table.str());
    }
}

TEST(PieceTableTest, InsertAtBeginningOfPiece1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    base::PieceTable table{str};

    const std::string s1 = "String1 ";
    str.insert(0, s1);
    table.insert(0, s1);
    EXPECT_EQ(str, table.str());

    const std::string s2 = "String2 ";
    str.insert(s1.length(), s2);
    table.insert(s1.length(), s2);
    EXPECT_EQ(str, table.str());
}

TEST(PieceTableTest, InsertAtBeginningOfPiece2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    base::PieceTable table{str};

    const std::string s1 = "String1 ";
    str.insert(0, s1);
    table.insert(0, s1);
    EXPECT_EQ(str, table.str());

    const std::string s2 = "String2 ";
    str.insert(s1.length(), s2);
    table.insert(s1.length(), s2);
    EXPECT_EQ(str, table.str());

    const std::string s3 = "String3 ";
    str.insert(s1.length(), s3);
    table.insert(s1.length(), s3);
    EXPECT_EQ(str, table.str());

    const std::string s4 = "String4 ";
    str.insert(s1.length() + s2.length(), s4);
    table.insert(s1.length() + s2.length(), s4);
    EXPECT_EQ(str, table.str());
}

TEST(PieceTableTest, InsertAtMiddleOfPiece1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    base::PieceTable table{str};

    const std::string s1 = "went to the park and\n";
    str.insert(20, s1);
    table.insert(20, s1);
    EXPECT_EQ(str, table.str());

    const std::string s2 = " and nimble";
    str.insert(9, s2);
    table.insert(9, s2);
    EXPECT_EQ(str, table.str());
}

TEST(PieceTableTest, InsertAtMiddleOfPiece2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    base::PieceTable table{str};

    const std::string s1 = "went to the park and\n";
    str.insert(20, s1);
    table.insert(20, s1);
    EXPECT_EQ(str, table.str());

    const std::string s2 = " and nimble";
    str.insert(9, s2);
    table.insert(9, s2);
    EXPECT_EQ(str, table.str());

    const std::string s3 = " sneaky,";
    str.insert(3, s3);
    table.insert(3, s3);
    EXPECT_EQ(str, table.str());

    const std::string s4 = "String4";
    str.insert(30, s4);
    table.insert(30, s4);
    EXPECT_EQ(str, table.str());

    const std::string s5 = "String5";
    str.insert(15, s5);
    table.insert(15, s5);
    EXPECT_EQ(str, table.str());
}

TEST(PieceTableTest, InsertAtEnd1) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    base::PieceTable table{str};

    const std::string s1 = " and walked away\n";
    size_t len = str.length();
    str.insert(len, s1);
    table.insert(len, s1);
    EXPECT_EQ(str, table.str());
}

TEST(PieceTableTest, InsertAtEnd2) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    base::PieceTable table{str};

    const std::string s1 = " and walked away\n";
    size_t len1 = str.length();
    str.insert(len1, s1);
    table.insert(len1, s1);
    EXPECT_EQ(str, table.str());

    const std::string s2 = "went to the park and\n";
    str.insert(20, s2);
    table.insert(20, s2);
    EXPECT_EQ(str, table.str());

    const std::string s3 = " and nimble";
    str.insert(9, s3);
    table.insert(9, s3);
    EXPECT_EQ(str, table.str());

    const std::string s4 = " from the dog\n";
    size_t len2 = str.length();
    str.insert(len2, s4);
    table.insert(len2, s4);
    EXPECT_EQ(str, table.str());
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

TEST(PieceTableTest, InsertRandom) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    base::PieceTable table{str};

    for (size_t i = 0; i < 100; i++) {
        const std::string random_str = RandomString(10);
        str.insert(0, random_str);
        table.insert(0, random_str);
        EXPECT_EQ(str, table.str());
    }
}

// TEST(PieceTableTest, EraseAtMiddleOfPiece1) {
//     std::string str = "The quick brown fox\njumped over the lazy dog";
//     base::PieceTable table{str};

//     str.erase(3, 6);
//     table.erase(3, 6);
//     EXPECT_EQ(str, table.str());

//     // std::cerr << str << '\n';
//     // std::cerr << table.str() << '\n';

//     // str.erase(4, 10);
//     // table.erase(4, 10);
//     // EXPECT_EQ(str, table.str());

//     std::cerr << table << '\n';
// }
}
