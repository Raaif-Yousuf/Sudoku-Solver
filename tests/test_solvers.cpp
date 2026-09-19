// Solver-agnostic behavior every registered sudoku::Solver must satisfy.
// Runs the same suite once per name returned by sudoku::solver_names(), so a
// newly registered solver is covered automatically.

#include <gtest/gtest.h>

#include <cctype>
#include <memory>
#include <string>
#include <string_view>

#include "sudoku/grid.hpp"
#include "sudoku/solver.hpp"

namespace {

using sudoku::Grid;
using sudoku::Solver;
using sudoku::SolveStats;

// A moderate, well-known puzzle (Wikipedia's example).
constexpr const char* kEasyPuzzle =
    "53..7....6..195....98....6.8...6...34..8.3..17...2...6.6....28....419..5....8..79";

// Arto Inkala's 2012 "world's hardest sudoku".
constexpr const char* kInkalaHardPuzzle =
    "8..........36......7..9.2...5...7.......457.....1...3...1....68..85...1..9....4..";

// Two puzzles from Peter Norvig's essay (norvig.com/sudoku.html). The second is
// his "hard1": it has many solutions and is a pathological case for search.
constexpr const char* kNorvigHardPuzzle1 =
    "4.....8.5.3..........7......2.....6.....8.4......1.......6.3.7.5..2.....1.4......";
constexpr const char* kNorvigHardPuzzle2 =
    ".....6....59.....82....8....45........3........6..3.54...325..6..................";

// A 17-clue puzzle (the minimum number of clues a valid Sudoku can have).
constexpr const char* kSeventeenCluePuzzle =
    "000000010400000000020000000000050407008000300001090000300400200050100000000806000";

// Consistent givens (no row/column/box clash) but no completion exists: cell
// (0, 0) can only be 1, 6, 7, 8, or 9 -- every arrangement of those leads to a
// dead end elsewhere in the grid.
constexpr const char* kConsistentButUnsolvablePuzzle =
    "030050040008010500400806001005000900700000008009000200200509007001070400080040010";

// A complete, valid solution (all rows/columns/boxes contain 1-9).
constexpr const char* kAlreadySolvedGrid =
    "534678912672195348198342567859761423426853791713924856961537284287419635345286179";

// Two givens in the same row: cell (0, 0) and (0, 1) both hold '5'.
constexpr const char* kInconsistentGivens =
    "55..7....6..195....98....6.8...6...34..8.3..17...2...6.6....28....419..5....8..79";

Grid MustParse(std::string_view text) {
    auto grid = sudoku::parse_grid(text);
    if (!grid.has_value()) {
        ADD_FAILURE() << "failed to parse puzzle: " << text;
        return Grid{};
    }
    return *grid;
}

// Turns a solver name into a valid googletest test-suite-instance suffix.
struct SolverNameToTestSuffix {
    std::string operator()(const ::testing::TestParamInfo<std::string_view>& info) const {
        std::string suffix(info.param);
        for (char& c : suffix) {
            if (std::isalnum(static_cast<unsigned char>(c)) == 0) {
                c = '_';
            }
        }
        return suffix;
    }
};

class SolverTest : public ::testing::TestWithParam<std::string_view> {
  protected:
    std::unique_ptr<Solver> MakeSolver() const {
        auto solver = sudoku::make_solver(GetParam());
        EXPECT_NE(solver, nullptr) << "make_solver(\"" << GetParam() << "\") returned nullptr";
        return solver;
    }

    // Asserts that `solver` finds a solution for `puzzle` that is both a
    // complete, valid grid and consistent with the puzzle's givens.
    static void ExpectSolves(Solver& solver, const Grid& puzzle) {
        Grid grid = puzzle;
        SolveStats stats;
        ASSERT_TRUE(solver.solve(grid, &stats)) << "solver=" << solver.name();
        EXPECT_TRUE(grid.is_solved()) << "solver=" << solver.name();
        EXPECT_TRUE(grid.extends(puzzle)) << "solver=" << solver.name();
    }

    // Asserts that `solver` reports no solution for `puzzle` and leaves the
    // grid passed to it unchanged.
    static void ExpectFailsWithoutModifying(Solver& solver, const Grid& puzzle) {
        Grid grid = puzzle;
        EXPECT_FALSE(solver.solve(grid)) << "solver=" << solver.name();
        EXPECT_EQ(grid, puzzle) << "solver=" << solver.name();
    }
};

TEST_P(SolverTest, NameMatchesFactoryParameter) {
    auto solver = MakeSolver();
    ASSERT_NE(solver, nullptr);
    EXPECT_EQ(solver->name(), GetParam());
}

TEST_P(SolverTest, SolvesEasyPuzzle) {
    auto solver = MakeSolver();
    ASSERT_NE(solver, nullptr);
    ExpectSolves(*solver, MustParse(kEasyPuzzle));
}

TEST_P(SolverTest, SolvesInkalaHardPuzzle) {
    auto solver = MakeSolver();
    ASSERT_NE(solver, nullptr);
    ExpectSolves(*solver, MustParse(kInkalaHardPuzzle));
}

TEST_P(SolverTest, SolvesNorvigHardPuzzleOne) {
    auto solver = MakeSolver();
    ASSERT_NE(solver, nullptr);
    ExpectSolves(*solver, MustParse(kNorvigHardPuzzle1));
}

TEST_P(SolverTest, SolvesNorvigHardPuzzleTwo) {
    auto solver = MakeSolver();
    ASSERT_NE(solver, nullptr);
    ExpectSolves(*solver, MustParse(kNorvigHardPuzzle2));
}

TEST_P(SolverTest, SolvesSeventeenCluePuzzle) {
    auto solver = MakeSolver();
    ASSERT_NE(solver, nullptr);
    Grid puzzle = MustParse(kSeventeenCluePuzzle);
    ASSERT_EQ(puzzle.filled_count(), 17);
    ExpectSolves(*solver, puzzle);
}

TEST_P(SolverTest, SolvesEmptyGrid) {
    auto solver = MakeSolver();
    ASSERT_NE(solver, nullptr);
    ExpectSolves(*solver, Grid{});
}

TEST_P(SolverTest, AlreadySolvedGridIsUnchanged) {
    auto solver = MakeSolver();
    ASSERT_NE(solver, nullptr);
    Grid solved = MustParse(kAlreadySolvedGrid);
    Grid grid = solved;
    ASSERT_TRUE(solver->solve(grid));
    EXPECT_EQ(grid, solved);
}

TEST_P(SolverTest, InconsistentGivensFailAndGridIsUnchanged) {
    auto solver = MakeSolver();
    ASSERT_NE(solver, nullptr);
    ExpectFailsWithoutModifying(*solver, MustParse(kInconsistentGivens));
}

TEST_P(SolverTest, ConsistentButUnsolvablePuzzleFails) {
    auto solver = MakeSolver();
    ASSERT_NE(solver, nullptr);
    Grid puzzle = MustParse(kConsistentButUnsolvablePuzzle);
    ASSERT_TRUE(puzzle.is_consistent());
    ExpectFailsWithoutModifying(*solver, puzzle);
}

TEST_P(SolverTest, NodeCountIsPositiveForNonTrivialPuzzle) {
    auto solver = MakeSolver();
    ASSERT_NE(solver, nullptr);
    Grid grid = MustParse(kInkalaHardPuzzle);
    SolveStats stats;
    ASSERT_TRUE(solver->solve(grid, &stats));
    EXPECT_GT(stats.nodes, 0u);
}

TEST_P(SolverTest, SolveAcceptsNullStats) {
    auto solver = MakeSolver();
    ASSERT_NE(solver, nullptr);
    Grid grid = MustParse(kEasyPuzzle);
    EXPECT_TRUE(solver->solve(grid, nullptr));
}

INSTANTIATE_TEST_SUITE_P(AllRegisteredSolvers, SolverTest,
                         ::testing::ValuesIn(sudoku::solver_names()), SolverNameToTestSuffix());

// ---------------------------------------------------------------------------
// Factory behavior that isn't tied to a particular solver instance.
// ---------------------------------------------------------------------------

TEST(SolverFactory, UnknownNameReturnsNullptr) {
    EXPECT_EQ(sudoku::make_solver("nope"), nullptr);
}

TEST(SolverFactory, EveryRegisteredNameConstructsAMatchingSolver) {
    for (std::string_view name : sudoku::solver_names()) {
        SCOPED_TRACE(name);
        auto solver = sudoku::make_solver(name);
        ASSERT_NE(solver, nullptr);
        EXPECT_EQ(solver->name(), name);
    }
}

}  // namespace
