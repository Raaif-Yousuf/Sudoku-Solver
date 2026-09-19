#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "sudoku/grid.hpp"

namespace {

constexpr const char* kPuzzle =
    "53..7....6..195....98....6.8...6...34..8.3..17...2...6.6....28....419..5....8..79";

constexpr const char* kPuzzleZeros =
    "530070000600195000098000060800060003400803001700020006060000280000419005000080079";

constexpr const char* kPuzzleRows =
    "5 3 . . 7 . . . .\n"
    "6 . . 1 9 5 . . .\n"
    ". 9 8 . . . . 6 .\n"
    "8 . . . 6 . . . 3\n"
    "4 . . 8 . 3 . . 1\n"
    "7 . . . 2 . . . 6\n"
    ". 6 . . . . 2 8 .\n"
    ". . . 4 1 9 . . 5\n"
    ". . . . 8 . . 7 9\n";

// ---------------------------------------------------------------------------
// parse_grid
// ---------------------------------------------------------------------------

TEST(ParseGrid, AcceptsEightyOneCharacterLine) {
    auto grid = sudoku::parse_grid(kPuzzle);
    ASSERT_TRUE(grid.has_value());
    EXPECT_EQ(grid->at(0, 0), 5);
    EXPECT_EQ(grid->at(0, 2), 0);
    EXPECT_EQ(grid->at(8, 8), 9);
    EXPECT_EQ(grid->to_line(), kPuzzle);
}

TEST(ParseGrid, DotAndZeroBlanksAreEquivalent) {
    auto dotted = sudoku::parse_grid(kPuzzle);
    auto zeroed = sudoku::parse_grid(kPuzzleZeros);
    ASSERT_TRUE(dotted.has_value());
    ASSERT_TRUE(zeroed.has_value());
    EXPECT_EQ(*dotted, *zeroed);
}

TEST(ParseGrid, AcceptsNineRowSpaceSeparatedFormat) {
    auto grid = sudoku::parse_grid(kPuzzleRows);
    auto reference = sudoku::parse_grid(kPuzzle);
    ASSERT_TRUE(grid.has_value());
    ASSERT_TRUE(reference.has_value());
    EXPECT_EQ(*grid, *reference);
}

TEST(ParseGrid, RoundTripsToPrettyOutput) {
    auto grid = sudoku::parse_grid(kPuzzle);
    ASSERT_TRUE(grid.has_value());
    auto reparsed = sudoku::parse_grid(grid->to_pretty());
    ASSERT_TRUE(reparsed.has_value());
    EXPECT_EQ(*grid, *reparsed);
}

TEST(ParseGrid, RejectsInvalidCharacter) {
    // 81 characters, but one of them ('x') is not a digit, '.', or ignored.
    std::string text(81, '.');
    text[10] = 'x';
    EXPECT_FALSE(sudoku::parse_grid(text).has_value());
}

TEST(ParseGrid, RejectsTooFewCells) {
    EXPECT_FALSE(sudoku::parse_grid("123").has_value());
    EXPECT_FALSE(sudoku::parse_grid(std::string(80, '.')).has_value());
}

TEST(ParseGrid, RejectsTooManyCells) {
    EXPECT_FALSE(sudoku::parse_grid(std::string(82, '.')).has_value());
}

// ---------------------------------------------------------------------------
// read_puzzles
// ---------------------------------------------------------------------------

TEST(ReadPuzzles, OnePerLineSkipsBlankAndCommentLines) {
    std::istringstream in(
        "# a leading comment\n"
        "\n" +
        std::string(kPuzzle) +
        "\n"
        "# a comment between puzzles\n" +
        std::string(kPuzzleZeros) +
        "\n"
        "\n");
    auto puzzles = sudoku::read_puzzles(in);
    ASSERT_TRUE(puzzles.has_value());
    ASSERT_EQ(puzzles->size(), 2u);
    EXPECT_EQ((*puzzles)[0], *sudoku::parse_grid(kPuzzle));
    EXPECT_EQ((*puzzles)[1], *sudoku::parse_grid(kPuzzleZeros));
}

TEST(ReadPuzzles, MultiLineSingleGrid) {
    std::istringstream in(kPuzzleRows);
    auto puzzles = sudoku::read_puzzles(in);
    ASSERT_TRUE(puzzles.has_value());
    ASSERT_EQ(puzzles->size(), 1u);
    EXPECT_EQ((*puzzles)[0], *sudoku::parse_grid(kPuzzle));
}

TEST(ReadPuzzles, RejectsMalformedInput) {
    std::istringstream in("not a puzzle at all\n");
    EXPECT_FALSE(sudoku::read_puzzles(in).has_value());
}

TEST(ReadPuzzles, RejectsOneValidAndOneMalformedLine) {
    std::istringstream in(std::string(kPuzzle) + "\nnot valid\n");
    EXPECT_FALSE(sudoku::read_puzzles(in).has_value());
}

TEST(ReadPuzzles, InputWithOnlyCommentsAndBlanksIsMalformed) {
    // No cells at all, so it can't be parsed as a single grid either.
    std::istringstream in("\n# only comments\n\n");
    EXPECT_FALSE(sudoku::read_puzzles(in).has_value());
}

// ---------------------------------------------------------------------------
// is_consistent
// ---------------------------------------------------------------------------

TEST(IsConsistent, TrueForEmptyGrid) {
    sudoku::Grid grid;
    EXPECT_TRUE(grid.is_consistent());
}

TEST(IsConsistent, TrueForValidPartialGrid) {
    auto grid = sudoku::parse_grid(kPuzzle);
    ASSERT_TRUE(grid.has_value());
    EXPECT_TRUE(grid->is_consistent());
}

TEST(IsConsistent, DetectsDuplicateInRow) {
    sudoku::Grid grid;
    grid.set(0, 0, 5);
    grid.set(0, 4, 5);
    EXPECT_FALSE(grid.is_consistent());
}

TEST(IsConsistent, DetectsDuplicateInColumn) {
    sudoku::Grid grid;
    grid.set(0, 0, 7);
    grid.set(6, 0, 7);
    EXPECT_FALSE(grid.is_consistent());
}

TEST(IsConsistent, DetectsDuplicateInBox) {
    sudoku::Grid grid;
    grid.set(0, 0, 4);
    grid.set(2, 2, 4);
    EXPECT_FALSE(grid.is_consistent());
}

TEST(IsConsistent, SameDigitInDifferentUnitsIsFine) {
    sudoku::Grid grid;
    grid.set(0, 0, 4);
    grid.set(4, 4, 4);
    grid.set(8, 8, 4);
    EXPECT_TRUE(grid.is_consistent());
}

// ---------------------------------------------------------------------------
// is_solved
// ---------------------------------------------------------------------------

TEST(IsSolved, FalseForPartialGrid) {
    auto grid = sudoku::parse_grid(kPuzzle);
    ASSERT_TRUE(grid.has_value());
    EXPECT_FALSE(grid->is_solved());
}

TEST(IsSolved, FalseForEmptyGrid) {
    sudoku::Grid grid;
    EXPECT_FALSE(grid.is_solved());
}

TEST(IsSolved, TrueForCompleteConsistentGrid) {
    auto grid = sudoku::parse_grid(
        "534678912"
        "672195348"
        "198342567"
        "859761423"
        "426853791"
        "713924856"
        "961537284"
        "287419635"
        "345286179");
    ASSERT_TRUE(grid.has_value());
    EXPECT_TRUE(grid->is_solved());
}

TEST(IsSolved, FalseForCompleteButInconsistentGrid) {
    // Every cell filled, but the first row has two 5s: not a valid solution.
    auto grid = sudoku::parse_grid(
        "554678912"
        "672195348"
        "198342567"
        "859761423"
        "426853791"
        "713924856"
        "961537284"
        "287419635"
        "345286179");
    ASSERT_TRUE(grid.has_value());
    EXPECT_FALSE(grid->is_solved());
}

// ---------------------------------------------------------------------------
// extends
// ---------------------------------------------------------------------------

TEST(Extends, TrueWhenAllGivensMatch) {
    auto puzzle = sudoku::parse_grid(kPuzzle);
    ASSERT_TRUE(puzzle.has_value());
    sudoku::Grid solution = *puzzle;
    solution.set(0, 2, 9);  // fill one of the blanks
    EXPECT_TRUE(solution.extends(*puzzle));
}

TEST(Extends, FalseWhenAGivenIsChanged) {
    auto puzzle = sudoku::parse_grid(kPuzzle);
    ASSERT_TRUE(puzzle.has_value());
    sudoku::Grid other = *puzzle;
    other.set(0, 0, 9);  // puzzle's (0,0) given is 5
    EXPECT_FALSE(other.extends(*puzzle));
}

TEST(Extends, EmptyGridExtendsAnyPuzzle) {
    auto puzzle = sudoku::parse_grid(kPuzzle);
    ASSERT_TRUE(puzzle.has_value());
    sudoku::Grid empty;
    EXPECT_TRUE(puzzle->extends(empty));
}

// ---------------------------------------------------------------------------
// filled_count
// ---------------------------------------------------------------------------

TEST(FilledCount, ZeroForEmptyGrid) {
    sudoku::Grid grid;
    EXPECT_EQ(grid.filled_count(), 0);
}

TEST(FilledCount, CountsNonEmptyCells) {
    auto grid = sudoku::parse_grid(kPuzzle);
    ASSERT_TRUE(grid.has_value());
    int expected = 0;
    for (int cell = 0; cell < sudoku::Grid::kCells; ++cell) {
        if (grid->at(cell) != 0) {
            ++expected;
        }
    }
    EXPECT_EQ(grid->filled_count(), expected);
    EXPECT_GT(grid->filled_count(), 0);
}

TEST(FilledCount, FullGridIsEightyOne) {
    auto grid = sudoku::parse_grid(
        "534678912"
        "672195348"
        "198342567"
        "859761423"
        "426853791"
        "713924856"
        "961537284"
        "287419635"
        "345286179");
    ASSERT_TRUE(grid.has_value());
    EXPECT_EQ(grid->filled_count(), sudoku::Grid::kCells);
}

// ---------------------------------------------------------------------------
// to_line
// ---------------------------------------------------------------------------

TEST(ToLine, RoundTripsThroughParseGrid) {
    auto grid = sudoku::parse_grid(kPuzzle);
    ASSERT_TRUE(grid.has_value());
    EXPECT_EQ(grid->to_line(), kPuzzle);

    auto reparsed = sudoku::parse_grid(grid->to_line());
    ASSERT_TRUE(reparsed.has_value());
    EXPECT_EQ(*reparsed, *grid);
}

TEST(ToLine, UsesDotForBlanks) {
    sudoku::Grid grid;
    EXPECT_EQ(grid.to_line(), std::string(sudoku::Grid::kCells, '.'));
}

}  // namespace
