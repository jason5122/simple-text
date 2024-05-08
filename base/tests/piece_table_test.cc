#include "base/piece_table.h"
#include "gtest/gtest.h"

TEST(PieceTableTest, Init) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable piece_table{str};
    EXPECT_EQ(str, piece_table.string());

    piece_table.printPieces();
}

TEST(PieceTableTest, Insert) {
    std::string str = "The quick brown fox\njumped over the lazy dog";
    PieceTable piece_table{str};

    std::string new_str;
    piece_table.insert(20, "went to the park and\n");
    new_str = "The quick brown fox\nwent to the park and\njumped over the lazy dog";
    EXPECT_EQ(new_str, piece_table.string());

    piece_table.insert(9, " and nimble");
    new_str = "The quick and nimble brown fox\nwent to the park and\njumped over the lazy dog";
    EXPECT_EQ(new_str, piece_table.string());

    piece_table.printPieces();
}
