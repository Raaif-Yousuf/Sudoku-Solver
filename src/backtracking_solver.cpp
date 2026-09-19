#include "sudoku/solver.hpp"

namespace sudoku {

namespace {

// Returns true if `number` can go in (row, column) without repeating a digit in
// that row, column, or 3x3 box.
bool is_valid(const Grid& grid, int row, int column, int number) {
    for (int i = 0; i < Grid::kSize; ++i) {
        if (grid.at(row, i) == number) {
            return false;
        }
    }

    for (int i = 0; i < Grid::kSize; ++i) {
        if (grid.at(i, column) == number) {
            return false;
        }
    }

    const int corner_row = row - row % Grid::kBox;
    const int corner_column = column - column % Grid::kBox;
    for (int i = 0; i < Grid::kBox; ++i) {
        for (int j = 0; j < Grid::kBox; ++j) {
            if (grid.at(corner_row + i, corner_column + j) == number) {
                return false;
            }
        }
    }

    return true;
}

// Fills empty cells in row-major order, trying 1-9 and undoing on dead ends.
bool solve_from(Grid& grid, int row, int column, SolveStats& stats) {
    if (column == Grid::kSize) {
        if (row == Grid::kSize - 1) {
            return true;
        }
        row += 1;
        column = 0;
    }

    if (grid.at(row, column) > 0) {
        return solve_from(grid, row, column + 1, stats);
    }

    for (int number = 1; number <= Grid::kSize; ++number) {
        if (is_valid(grid, row, column, number)) {
            ++stats.nodes;
            grid.set(row, column, number);
            if (solve_from(grid, row, column + 1, stats)) {
                return true;
            }
            grid.set(row, column, 0);
        }
    }

    return false;
}

}  // namespace

bool BacktrackingSolver::solve(Grid& grid, SolveStats* stats) {
    SolveStats local;
    SolveStats& counters = stats != nullptr ? *stats : local;

    // The search only checks new placements, so reject clashing givens up front.
    if (!grid.is_consistent()) {
        return false;
    }

    Grid work = grid;
    if (!solve_from(work, 0, 0, counters)) {
        return false;
    }
    grid = work;
    return true;
}

}  // namespace sudoku
