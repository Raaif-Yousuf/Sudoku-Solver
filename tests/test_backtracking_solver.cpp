#include <gtest/gtest.h>

#include "sudoku/grid.hpp"
#include "sudoku/solver.hpp"

namespace {

TEST(BacktrackingSolver, SolvesClassicPuzzle) {
    auto puzzle = sudoku::parse_grid(
        "53..7....6..195....98....6.8...6...34..8.3..17...2...6.6....28....419..5....8..79");
    ASSERT_TRUE(puzzle.has_value());

    sudoku::Grid grid = *puzzle;
    sudoku::BacktrackingSolver solver;
    ASSERT_TRUE(solver.solve(grid));
    EXPECT_TRUE(grid.is_solved());
    EXPECT_TRUE(grid.extends(*puzzle));
}

}  // namespace
