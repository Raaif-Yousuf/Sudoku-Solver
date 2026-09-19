#include <gtest/gtest.h>

#include "sudoku/grid.hpp"

namespace {

constexpr const char* kPuzzle =
    "53..7....6..195....98....6.8...6...34..8.3..17...2...6.6....28....419..5....8..79";

TEST(ParseGrid, AcceptsEightyOneCharacterLine) {
    auto grid = sudoku::parse_grid(kPuzzle);
    ASSERT_TRUE(grid.has_value());
    EXPECT_EQ(grid->at(0, 0), 5);
    EXPECT_EQ(grid->at(0, 2), 0);
    EXPECT_EQ(grid->at(8, 8), 9);
    EXPECT_EQ(grid->to_line(), kPuzzle);
}

TEST(ParseGrid, RejectsWrongCellCount) {
    EXPECT_FALSE(sudoku::parse_grid("123").has_value());
}

}  // namespace
