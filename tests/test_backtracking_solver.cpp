// Behavior specific to the concrete BacktrackingSolver type, as opposed to
// the sudoku::Solver interface in general (see test_solvers.cpp for that).

#include <gtest/gtest.h>

#include "sudoku/grid.hpp"
#include "sudoku/solver.hpp"

namespace {

// BacktrackingSolver can be used directly, without going through
// make_solver(). This exercises that path, since test_solvers.cpp only ever
// constructs solvers through the factory.
TEST(BacktrackingSolver, UsableWithoutTheFactory) {
    auto puzzle = sudoku::parse_grid(
        "53..7....6..195....98....6.8...6...34..8.3..17...2...6.6....28....419..5....8..79");
    ASSERT_TRUE(puzzle.has_value());

    sudoku::Grid grid = *puzzle;
    sudoku::BacktrackingSolver solver;
    EXPECT_EQ(solver.name(), "backtracking");
    ASSERT_TRUE(solver.solve(grid));
    EXPECT_TRUE(grid.is_solved());
    EXPECT_TRUE(grid.extends(*puzzle));
}

}  // namespace
