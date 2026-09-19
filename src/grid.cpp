#include "sudoku/grid.hpp"

#include <istream>
#include <ostream>
#include <sstream>

namespace sudoku {

namespace {

enum class CharKind { kCell, kIgnored, kInvalid };

CharKind classify(char c) {
    if ((c >= '0' && c <= '9') || c == '.') {
        return CharKind::kCell;
    }
    switch (c) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '|':
        case '-':
        case '+':
            return CharKind::kIgnored;
        default:
            return CharKind::kInvalid;
    }
}

// Counts cell characters in `line`, or returns -1 if an invalid character appears.
int count_cells(std::string_view line) {
    int cells = 0;
    for (char c : line) {
        switch (classify(c)) {
            case CharKind::kCell:
                ++cells;
                break;
            case CharKind::kIgnored:
                break;
            case CharKind::kInvalid:
                return -1;
        }
    }
    return cells;
}

bool is_skippable(std::string_view line) {
    const auto first = line.find_first_not_of(" \t\r\n");
    return first == std::string_view::npos || line[first] == '#';
}

}  // namespace

int Grid::filled_count() const {
    int count = 0;
    for (auto value : cells_) {
        if (value != 0) {
            ++count;
        }
    }
    return count;
}

bool Grid::is_consistent() const {
    for (int unit = 0; unit < kSize; ++unit) {
        bool row_seen[kSize + 1] = {};
        bool col_seen[kSize + 1] = {};
        bool box_seen[kSize + 1] = {};
        const int box_row = (unit / kBox) * kBox;
        const int box_col = (unit % kBox) * kBox;
        for (int i = 0; i < kSize; ++i) {
            const int r = at(unit, i);
            const int c = at(i, unit);
            const int b = at(box_row + i / kBox, box_col + i % kBox);
            if ((r != 0 && row_seen[r]) || (c != 0 && col_seen[c]) || (b != 0 && box_seen[b])) {
                return false;
            }
            row_seen[r] = true;
            col_seen[c] = true;
            box_seen[b] = true;
        }
    }
    return true;
}

bool Grid::is_solved() const {
    return filled_count() == kCells && is_consistent();
}

bool Grid::extends(const Grid& puzzle) const {
    for (int cell = 0; cell < kCells; ++cell) {
        if (puzzle.at(cell) != 0 && puzzle.at(cell) != at(cell)) {
            return false;
        }
    }
    return true;
}

std::string Grid::to_line() const {
    std::string line(kCells, '.');
    for (int cell = 0; cell < kCells; ++cell) {
        if (at(cell) != 0) {
            line[static_cast<std::size_t>(cell)] = static_cast<char>('0' + at(cell));
        }
    }
    return line;
}

std::string Grid::to_pretty() const {
    std::ostringstream out;
    for (int row = 0; row < kSize; ++row) {
        for (int col = 0; col < kSize; ++col) {
            if (col == 3 || col == 6) {
                out << "| ";
            }
            const int value = at(row, col);
            out << (value == 0 ? '.' : static_cast<char>('0' + value));
            out << (col == kSize - 1 ? "\n" : " ");
        }
        if (row == 2 || row == 5) {
            out << "------+-------+------\n";
        }
    }
    return out.str();
}

std::optional<Grid> parse_grid(std::string_view text) {
    Grid grid;
    int cell = 0;
    for (char c : text) {
        switch (classify(c)) {
            case CharKind::kIgnored:
                continue;
            case CharKind::kInvalid:
                return std::nullopt;
            case CharKind::kCell:
                if (cell == Grid::kCells) {
                    return std::nullopt;
                }
                grid.set(cell++, c == '.' ? 0 : c - '0');
                break;
        }
    }
    if (cell != Grid::kCells) {
        return std::nullopt;
    }
    return grid;
}

std::optional<std::vector<Grid>> read_puzzles(std::istream& in) {
    std::vector<std::string> lines;
    for (std::string line; std::getline(in, line);) {
        if (!is_skippable(line)) {
            lines.push_back(line);
        }
    }

    bool one_per_line = !lines.empty();
    for (const auto& line : lines) {
        if (count_cells(line) != Grid::kCells) {
            one_per_line = false;
            break;
        }
    }

    std::vector<Grid> puzzles;
    if (one_per_line) {
        for (const auto& line : lines) {
            auto grid = parse_grid(line);
            if (!grid) {
                return std::nullopt;
            }
            puzzles.push_back(*grid);
        }
        return puzzles;
    }

    std::string joined;
    for (const auto& line : lines) {
        joined += line;
        joined += '\n';
    }
    auto grid = parse_grid(joined);
    if (!grid) {
        return std::nullopt;
    }
    puzzles.push_back(*grid);
    return puzzles;
}

std::ostream& operator<<(std::ostream& os, const Grid& grid) {
    return os << grid.to_line();
}

}  // namespace sudoku
