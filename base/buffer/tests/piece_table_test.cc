#include "base/buffer/piece_table.h"
#include "gtest/gtest.h"

TEST(PieceTableTest, Init) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    base::PieceTable piece_table{str};
    EXPECT_EQ(str, piece_table.str());
}

TEST(PieceTableTest, Insert) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    base::PieceTable piece_table{str};

    str.insert(20, "went to the park and\n");
    piece_table.insert(20, "went to the park and\n");
    EXPECT_EQ(str, piece_table.str());

    str.insert(9, " and nimble");
    piece_table.insert(9, " and nimble");
    EXPECT_EQ(str, piece_table.str());

    // std::cerr << piece_table << '\n';
}

TEST(PieceTableTest, Erase) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    base::PieceTable piece_table{str};

    str.erase(3, 6);
    piece_table.erase(3, 6);
    EXPECT_EQ(str, piece_table.str());

    // std::cerr << str << '\n';
    // std::cerr << piece_table.str() << '\n';

    // str.erase(4, 10);
    // piece_table.erase(4, 10);
    // EXPECT_EQ(str, piece_table.str());

    std::cerr << piece_table << '\n';
}
