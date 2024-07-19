#include "base/buffer/piece_table.h"
#include "gtest/gtest.h"

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

    std::cerr << str << '\n';
    std::cerr << table << '\n';

    const std::string s1 = "went to the park and\n";
    str.insert(20, s1);
    table.insert(20, s1);
    EXPECT_EQ(str, table.str());

    std::cerr << str << '\n';
    std::cerr << table << '\n';

    const std::string s2 = " and nimble";
    str.insert(9, s2);
    table.insert(9, s2);
    EXPECT_EQ(str, table.str());

    std::cerr << str << '\n';
    std::cerr << table << '\n';

    const std::string s3 = " sneaky,";
    str.insert(3, s3);
    table.insert(3, s3);
    EXPECT_EQ(str, table.str());

    std::cerr << str << '\n';
    std::cerr << table << '\n';

    const std::string s4 = "String4";
    str.insert(30, s4);
    table.insert(30, s4);
    EXPECT_EQ(str, table.str());

    std::cerr << str << '\n';
    std::cerr << table << '\n';

    // std::string str2 = "The sneaky, quick and nimble brown fox\nwent to the park and\njumped "
    //                    "over the lazy dog";
    // PieceTable table2{str2};

    // const std::string s4 = "String4";
    // str2.insert(30, s4);
    // table2.insert(30, s4);
    // EXPECT_EQ(str2, table2.str());

    // std::cerr << str2 << '\n';
    // std::cerr << table2 << '\n';

    // str.insert(15, s2);
    // table.insert(15, s2);
    // EXPECT_EQ(str, table.str());

    // std::cerr << table << '\n';
}

// TEST(PieceTableTest, InsertAtEnd1) {
//     std::string str = "The quick brown fox\njumped over the lazy dog";
//     base::PieceTable table{str};

//     const std::string s1 = " and walked away\n";
//     size_t len = str.length();
//     str.insert(len, s1);
//     table.insert(len, s1);
//     EXPECT_EQ(str, table.str());

//     std::cerr << table << '\n';
// }

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
