#pragma once

#include <array>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sudoku {

/// A 9x9 Sudoku board. Empty cells hold 0, filled cells hold 1-9.
class Grid {
  public:
    static constexpr int kSize = 9;
    static constexpr int kBox = 3;
    static constexpr int kCells = kSize * kSize;

    Grid() = default;

    [[nodiscard]] int at(int row, int col) const { return cells_[index(row, col)]; }
    void set(int row, int col, int value) {
        cells_[index(row, col)] = static_cast<std::uint8_t>(value);
    }

    [[nodiscard]] int at(int cell) const { return cells_[static_cast<std::size_t>(cell)]; }
    void set(int cell, int value) {
        cells_[static_cast<std::size_t>(cell)] = static_cast<std::uint8_t>(value);
    }

    /// Number of non-empty cells.
    [[nodiscard]] int filled_count() const;

    /// True if no row, column, or box contains the same digit twice (empty cells are ignored).
    [[nodiscard]] bool is_consistent() const;

    /// True if every cell is filled and the grid is consistent.
    [[nodiscard]] bool is_solved() const;

    /// True if every given (non-zero) cell of `puzzle` has the same value in this grid.
    [[nodiscard]] bool extends(const Grid& puzzle) const;

    /// Compact 81-character form, '.' for empty cells.
    [[nodiscard]] std::string to_line() const;

    /// Human-readable 9x9 layout with box separators.
    [[nodiscard]] std::string to_pretty() const;

    friend bool operator==(const Grid& a, const Grid& b) { return a.cells_ == b.cells_; }
    friend bool operator!=(const Grid& a, const Grid& b) { return !(a == b); }

  private:
    static constexpr std::size_t index(int row, int col) {
        return static_cast<std::size_t>(row * kSize + col);
    }

    std::array<std::uint8_t, kCells> cells_{};
};

/// Parses a single puzzle. Digits 1-9 are givens; '0' and '.' are empty cells.
/// Whitespace and the box-drawing characters '|', '-', '+' are ignored, so this
/// accepts the 81-character line format, 9 rows of space-separated digits, and
/// the output of Grid::to_pretty(). Returns std::nullopt unless exactly 81 cells
/// are found and no other characters appear.
[[nodiscard]] std::optional<Grid> parse_grid(std::string_view text);

/// Reads every puzzle from a stream. If each non-empty line (ignoring lines that
/// start with '#') holds exactly 81 cells, each line is one puzzle; otherwise the
/// whole input is parsed as a single puzzle. Returns std::nullopt on malformed input.
[[nodiscard]] std::optional<std::vector<Grid>> read_puzzles(std::istream& in);

std::ostream& operator<<(std::ostream& os, const Grid& grid);

}  // namespace sudoku
