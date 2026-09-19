#include <gtest/gtest.h>

#include "sudoku/grid.hpp"
#include "sudoku/solver.hpp"

namespace {

constexpr const char* kEasyPuzzle =
    "53..7....6..195....98....6.8...6...34..8.3..17...2...6.6....28....419..5....8..79";

// Arto Inkala's widely cited "hardest sudoku".
constexpr const char* kInkalaPuzzle =
    "8..........36......7..9.2...5...7.......457.....1...3...1....68..85...1..9....4..";

// One of Peter Norvig's "hardest" example puzzles (see norvig.com/sudoku.html).
constexpr const char* kNorvigHardPuzzle =
    "4.....8.5.3..........7......2.....6.....8.4......1.......6.3.7.5..2.....1.4......";

// Consistent (no clashing givens) but has no completion.
constexpr const char* kUnsolvablePuzzle =
    "030050040008010500400806001005000900700000008009000200200509007001070400080040010";

// Two givens repeat 5 in the same row.
constexpr const char* kInconsistentPuzzle =
    "553......6..195....98....6.8...6...34..8.3..17...2...6.6....28....419..5....8..79";

sudoku::Grid parse(const char* text) {
    auto grid = sudoku::parse_grid(text);
    if (!grid.has_value()) {
        ADD_FAILURE() << "failed to parse puzzle: " << text;
        return sudoku::Grid();
    }
    return *grid;
}

TEST(FastSolver, SolvesEasyPuzzle) {
    const sudoku::Grid puzzle = parse(kEasyPuzzle);
    sudoku::Grid grid = puzzle;
    sudoku::FastSolver solver;
    ASSERT_TRUE(solver.solve(grid));
    EXPECT_TRUE(grid.is_solved());
    EXPECT_TRUE(grid.extends(puzzle));
}

TEST(FastSolver, SolvesInkalaPuzzle) {
    const sudoku::Grid puzzle = parse(kInkalaPuzzle);
    sudoku::Grid grid = puzzle;
    sudoku::FastSolver solver;
    ASSERT_TRUE(solver.solve(grid));
    EXPECT_TRUE(grid.is_solved());
    EXPECT_TRUE(grid.extends(puzzle));
}

TEST(FastSolver, SolvesNorvigHardPuzzle) {
    const sudoku::Grid puzzle = parse(kNorvigHardPuzzle);
    sudoku::Grid grid = puzzle;
    sudoku::FastSolver solver;
    ASSERT_TRUE(solver.solve(grid));
    EXPECT_TRUE(grid.is_solved());
    EXPECT_TRUE(grid.extends(puzzle));
}

TEST(FastSolver, MatchesBacktrackingOnUniquePuzzles) {
    for (const char* text : {kEasyPuzzle, kInkalaPuzzle, kNorvigHardPuzzle}) {
        const sudoku::Grid puzzle = parse(text);

        sudoku::Grid fast_result = puzzle;
        sudoku::FastSolver fast_solver;
        ASSERT_TRUE(fast_solver.solve(fast_result));

        sudoku::Grid backtracking_result = puzzle;
        sudoku::BacktrackingSolver backtracking_solver;
        ASSERT_TRUE(backtracking_solver.solve(backtracking_result));

        EXPECT_EQ(fast_result, backtracking_result);
    }
}

TEST(FastSolver, RejectsInconsistentGivens) {
    const sudoku::Grid puzzle = parse(kInconsistentPuzzle);
    ASSERT_FALSE(puzzle.is_consistent());

    sudoku::Grid grid = puzzle;
    sudoku::FastSolver solver;
    EXPECT_FALSE(solver.solve(grid));
    EXPECT_EQ(grid, puzzle);
}

TEST(FastSolver, RejectsConsistentButUnsolvablePuzzle) {
    const sudoku::Grid puzzle = parse(kUnsolvablePuzzle);
    ASSERT_TRUE(puzzle.is_consistent());

    sudoku::Grid grid = puzzle;
    sudoku::FastSolver solver;
    EXPECT_FALSE(solver.solve(grid));
    EXPECT_EQ(grid, puzzle);
}

TEST(FastSolver, SolvesEmptyGrid) {
    sudoku::Grid grid;
    sudoku::FastSolver solver;
    ASSERT_TRUE(solver.solve(grid));
    EXPECT_TRUE(grid.is_solved());
}

TEST(FastSolver, UsesFewerNodesThanBacktrackingOnHardPuzzle) {
    const sudoku::Grid puzzle = parse(kInkalaPuzzle);

    sudoku::Grid fast_grid = puzzle;
    sudoku::SolveStats fast_stats;
    sudoku::FastSolver fast_solver;
    ASSERT_TRUE(fast_solver.solve(fast_grid, &fast_stats));

    sudoku::Grid backtracking_grid = puzzle;
    sudoku::SolveStats backtracking_stats;
    sudoku::BacktrackingSolver backtracking_solver;
    ASSERT_TRUE(backtracking_solver.solve(backtracking_grid, &backtracking_stats));

    EXPECT_LT(fast_stats.nodes, backtracking_stats.nodes);
}

TEST(CountSolutions, ReturnsOneForUniquePuzzle) {
    const sudoku::Grid puzzle = parse(kInkalaPuzzle);
    EXPECT_EQ(sudoku::count_solutions(puzzle), 1u);
}

TEST(CountSolutions, CapsAtLimitForEmptyGrid) {
    const sudoku::Grid grid;
    EXPECT_EQ(sudoku::count_solutions(grid, 2), 2u);
}

TEST(CountSolutions, ReturnsZeroForUnsolvablePuzzle) {
    const sudoku::Grid puzzle = parse(kUnsolvablePuzzle);
    EXPECT_EQ(sudoku::count_solutions(puzzle), 0u);
}

TEST(CountSolutions, ReturnsZeroForInconsistentPuzzle) {
    const sudoku::Grid puzzle = parse(kInconsistentPuzzle);
    EXPECT_EQ(sudoku::count_solutions(puzzle), 0u);
}

}  // namespace
