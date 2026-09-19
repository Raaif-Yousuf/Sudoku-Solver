#include "sudoku/solver.hpp"

#include <array>
#include <optional>
#include <utility>

namespace sudoku {

namespace {

using Mask = std::uint16_t;
constexpr Mask kFullMask = 0x1FFu;  // Bits 0-8 stand for digits 1-9.

enum class UnitKind { kRow, kCol, kBox };

constexpr int box_of(int row, int col) {
    return (row / Grid::kBox) * Grid::kBox + col / Grid::kBox;
}

// Row/column of the i-th cell (0-8) of the given unit.
constexpr int unit_row(UnitKind kind, int unit, int i) {
    switch (kind) {
        case UnitKind::kRow:
            return unit;
        case UnitKind::kCol:
            return i;
        case UnitKind::kBox:
            return (unit / Grid::kBox) * Grid::kBox + i / Grid::kBox;
    }
    return 0;
}

constexpr int unit_col(UnitKind kind, int unit, int i) {
    switch (kind) {
        case UnitKind::kRow:
            return i;
        case UnitKind::kCol:
            return unit;
        case UnitKind::kBox:
            return (unit % Grid::kBox) * Grid::kBox + i % Grid::kBox;
    }
    return 0;
}

// Lowest set bit of `mask` (0 if `mask` is 0).
[[nodiscard]] constexpr Mask low_bit(Mask mask) {
    return static_cast<Mask>(mask & static_cast<Mask>(0u - mask));
}

[[nodiscard]] constexpr int popcount(Mask mask) {
    int count = 0;
    while (mask != 0) {
        mask = static_cast<Mask>(mask & static_cast<Mask>(mask - 1));
        ++count;
    }
    return count;
}

// `mask` must have exactly one bit set; returns the digit (1-9) it stands for.
[[nodiscard]] constexpr int bit_to_digit(Mask mask) {
    int digit = 1;
    while ((mask & 1u) == 0u) {
        mask = static_cast<Mask>(mask >> 1);
        ++digit;
    }
    return digit;
}

// Small, copyable snapshot of the solving state: filled digits plus the used-
// digit masks for every row, column, and box. Copying this is how branching
// works, so it holds only fixed-size arrays and never allocates.
struct FastState {
    std::array<std::uint8_t, static_cast<std::size_t>(Grid::kCells)> cells{};
    std::array<Mask, static_cast<std::size_t>(Grid::kSize)> row_used{};
    std::array<Mask, static_cast<std::size_t>(Grid::kSize)> col_used{};
    std::array<Mask, static_cast<std::size_t>(Grid::kSize)> box_used{};

    [[nodiscard]] static std::size_t cell_index(int row, int col) {
        return static_cast<std::size_t>(row * Grid::kSize + col);
    }

    [[nodiscard]] int at(int row, int col) const { return cells[cell_index(row, col)]; }

    [[nodiscard]] Mask candidates(int row, int col) const {
        const Mask used = row_used[static_cast<std::size_t>(row)] |
                           col_used[static_cast<std::size_t>(col)] |
                           box_used[static_cast<std::size_t>(box_of(row, col))];
        return static_cast<Mask>(~used & kFullMask);
    }

    // Places `digit` (1-9) at (row, col). The caller must have already
    // checked that the cell is empty and the digit a legal candidate there.
    void place(int row, int col, int digit) {
        const Mask bit = static_cast<Mask>(1u << (digit - 1));
        cells[cell_index(row, col)] = static_cast<std::uint8_t>(digit);
        row_used[static_cast<std::size_t>(row)] |= bit;
        col_used[static_cast<std::size_t>(col)] |= bit;
        box_used[static_cast<std::size_t>(box_of(row, col))] |= bit;
    }
};

// Builds the initial state from `grid`, or returns std::nullopt if two givens
// clash in a row, column, or box.
[[nodiscard]] std::optional<FastState> make_state(const Grid& grid) {
    FastState state;
    for (int row = 0; row < Grid::kSize; ++row) {
        for (int col = 0; col < Grid::kSize; ++col) {
            const int value = grid.at(row, col);
            if (value == 0) {
                continue;
            }
            const Mask bit = static_cast<Mask>(1u << (value - 1));
            const Mask used = state.row_used[static_cast<std::size_t>(row)] |
                               state.col_used[static_cast<std::size_t>(col)] |
                               state.box_used[static_cast<std::size_t>(box_of(row, col))];
            if ((used & bit) != 0) {
                return std::nullopt;
            }
            state.place(row, col, value);
        }
    }
    return state;
}

[[nodiscard]] Grid to_grid(const FastState& state) {
    Grid grid;
    for (int cell = 0; cell < Grid::kCells; ++cell) {
        grid.set(cell, state.cells[static_cast<std::size_t>(cell)]);
    }
    return grid;
}

// Finds every digit that has exactly one legal cell left within one unit
// (row, column, or box) and places it. Returns false if some digit has no
// legal cell left in a unit it is not already placed in.
[[nodiscard]] bool apply_hidden_singles(FastState& state, UnitKind kind, bool& changed) {
    for (int unit = 0; unit < Grid::kSize; ++unit) {
        for (int digit = 1; digit <= Grid::kSize; ++digit) {
            const Mask bit = static_cast<Mask>(1u << (digit - 1));
            bool already_placed = false;
            int candidate_count = 0;
            int found_row = -1;
            int found_col = -1;
            for (int i = 0; i < Grid::kSize; ++i) {
                const int row = unit_row(kind, unit, i);
                const int col = unit_col(kind, unit, i);
                if (state.at(row, col) == digit) {
                    already_placed = true;
                    break;
                }
                if (state.at(row, col) == 0 && (state.candidates(row, col) & bit) != 0) {
                    ++candidate_count;
                    found_row = row;
                    found_col = col;
                }
            }
            if (already_placed) {
                continue;
            }
            if (candidate_count == 0) {
                return false;
            }
            if (candidate_count == 1) {
                state.place(found_row, found_col, digit);
                changed = true;
            }
        }
    }
    return true;
}

// Repeatedly resolves naked and hidden singles until neither applies. Returns
// false as soon as a cell or unit proves the state inconsistent.
[[nodiscard]] bool propagate(FastState& state) {
    bool changed = true;
    while (changed) {
        changed = false;

        for (int row = 0; row < Grid::kSize; ++row) {
            for (int col = 0; col < Grid::kSize; ++col) {
                if (state.at(row, col) != 0) {
                    continue;
                }
                const Mask cands = state.candidates(row, col);
                if (cands == 0) {
                    return false;
                }
                if ((cands & static_cast<Mask>(cands - 1)) == 0) {
                    state.place(row, col, bit_to_digit(cands));
                    changed = true;
                }
            }
        }

        if (!apply_hidden_singles(state, UnitKind::kRow, changed) ||
            !apply_hidden_singles(state, UnitKind::kCol, changed) ||
            !apply_hidden_singles(state, UnitKind::kBox, changed)) {
            return false;
        }
    }
    return true;
}

// Finds the empty cell with the fewest remaining candidates. Returns false
// (leaving the outputs untouched) if there is no empty cell.
[[nodiscard]] bool find_mrv_cell(const FastState& state, int& out_row, int& out_col,
                                  Mask& out_cands) {
    int best_count = Grid::kSize + 1;
    for (int row = 0; row < Grid::kSize; ++row) {
        for (int col = 0; col < Grid::kSize; ++col) {
            if (state.at(row, col) != 0) {
                continue;
            }
            const Mask cands = state.candidates(row, col);
            const int count = popcount(cands);
            if (count < best_count) {
                best_count = count;
                out_row = row;
                out_col = col;
                out_cands = cands;
            }
        }
    }
    return best_count <= Grid::kSize;
}

// Depth-first search with constraint propagation at every node. On success,
// `result` holds the completed grid. Each tentative placement made while
// branching (i.e. every candidate digit actually tried) counts as one node.
[[nodiscard]] bool search(FastState state, SolveStats* stats, FastState& result) {
    if (!propagate(state)) {
        return false;
    }

    int row = 0;
    int col = 0;
    Mask cands = 0;
    if (!find_mrv_cell(state, row, col, cands)) {
        result = state;
        return true;
    }

    while (cands != 0) {
        const Mask bit = low_bit(cands);
        cands = static_cast<Mask>(cands ^ bit);

        FastState branch = state;
        branch.place(row, col, bit_to_digit(bit));
        if (stats != nullptr) {
            ++stats->nodes;
        }
        if (search(std::move(branch), stats, result)) {
            return true;
        }
    }
    return false;
}

// Same search as above, but keeps branching until `limit` solutions have
// been found instead of stopping at the first one.
void count_search(FastState state, std::size_t limit, std::size_t& found) {
    if (found >= limit || !propagate(state)) {
        return;
    }

    int row = 0;
    int col = 0;
    Mask cands = 0;
    if (!find_mrv_cell(state, row, col, cands)) {
        ++found;
        return;
    }

    while (cands != 0 && found < limit) {
        const Mask bit = low_bit(cands);
        cands = static_cast<Mask>(cands ^ bit);

        FastState branch = state;
        branch.place(row, col, bit_to_digit(bit));
        count_search(std::move(branch), limit, found);
    }
}

}  // namespace

bool FastSolver::solve(Grid& grid, SolveStats* stats) {
    auto initial = make_state(grid);
    if (!initial) {
        return false;
    }

    FastState result;
    if (!search(std::move(*initial), stats, result)) {
        return false;
    }
    grid = to_grid(result);
    return true;
}

std::size_t count_solutions(const Grid& puzzle, std::size_t limit) {
    if (limit == 0) {
        return 0;
    }

    auto initial = make_state(puzzle);
    if (!initial) {
        return 0;
    }

    std::size_t found = 0;
    count_search(std::move(*initial), limit, found);
    return found;
}

}  // namespace sudoku
