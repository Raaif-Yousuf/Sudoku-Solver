#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "sudoku/grid.hpp"
#include "sudoku/solver.hpp"

namespace {

constexpr int kExitSolved = 0;
constexpr int kExitUnsolvable = 1;
constexpr int kExitUsage = 2;

enum class Format { kAuto, kPretty, kLine };

struct Options {
    std::string solver = "backtracking";
    Format format = Format::kAuto;
    bool stats = false;
    bool interactive = false;
    std::string path;  // empty or "-" means stdin
};

void print_usage(std::ostream& os) {
    os << "Usage: sudoku [options] [FILE]\n"
          "\n"
          "Solves Sudoku puzzles read from FILE, or from standard input if FILE is\n"
          "omitted or '-'. Input is either one puzzle per line (81 characters, '0' or\n"
          "'.' for blanks) or a single 9x9 grid of digits.\n"
          "\n"
          "Options:\n"
          "  -s, --solver NAME   solving strategy:";
    for (auto name : sudoku::solver_names()) {
        os << ' ' << name;
    }
    os << " (default: backtracking)\n"
          "  -f, --format FMT    output format: pretty or line (default: pretty for one\n"
          "                      puzzle, line for several)\n"
          "      --stats         print search nodes and solve time to stderr\n"
          "  -i, --interactive   prompt for the puzzle row by row\n"
          "  -h, --help          show this message\n"
          "\n"
          "Exit status: 0 if every puzzle was solved, 1 if any had no solution,\n"
          "2 on invalid usage or input.\n";
}

// Returns false (after printing a message) if the arguments are invalid.
bool parse_args(int argc, char** argv, Options& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        auto next_value = [&](std::string& out) {
            if (i + 1 >= argc) {
                std::cerr << "sudoku: " << arg << " needs a value\n";
                return false;
            }
            out = argv[++i];
            return true;
        };

        if (arg == "-h" || arg == "--help") {
            print_usage(std::cout);
            std::exit(kExitSolved);
        } else if (arg == "-s" || arg == "--solver") {
            if (!next_value(options.solver)) {
                return false;
            }
        } else if (arg == "-f" || arg == "--format") {
            std::string value;
            if (!next_value(value)) {
                return false;
            }
            if (value == "pretty") {
                options.format = Format::kPretty;
            } else if (value == "line") {
                options.format = Format::kLine;
            } else {
                std::cerr << "sudoku: unknown format '" << value << "'\n";
                return false;
            }
        } else if (arg == "--stats") {
            options.stats = true;
        } else if (arg == "-i" || arg == "--interactive") {
            options.interactive = true;
        } else if (arg.size() > 1 && arg[0] == '-') {
            std::cerr << "sudoku: unknown option '" << arg << "'\n";
            return false;
        } else if (options.path.empty()) {
            options.path = std::string(arg);
        } else {
            std::cerr << "sudoku: more than one input file given\n";
            return false;
        }
    }
    return true;
}

// The original row-by-row prompt: nine lines of nine space-separated digits.
std::optional<sudoku::Grid> read_interactive() {
    sudoku::Grid grid;
    std::cout << "Enter the Sudoku puzzle row by row (use 0 for empty cells):\n";
    for (int row = 0; row < sudoku::Grid::kSize; ++row) {
        while (true) {
            std::cout << "Row " << row + 1 << " (9 numbers, space-separated): " << std::flush;
            std::string line;
            if (!std::getline(std::cin, line)) {
                return std::nullopt;
            }
            std::istringstream iss(line);
            int values[sudoku::Grid::kSize];
            bool valid = true;
            for (int& value : values) {
                if (!(iss >> value) || value < 0 || value > 9) {
                    valid = false;
                    break;
                }
            }
            std::string extra;
            if (valid && !(iss >> extra)) {
                for (int col = 0; col < sudoku::Grid::kSize; ++col) {
                    grid.set(row, col, values[col]);
                }
                break;
            }
            std::cout << "Invalid input! Enter exactly 9 numbers between 0 and 9.\n";
        }
    }
    return grid;
}

}  // namespace

int main(int argc, char** argv) {
    Options options;
    if (!parse_args(argc, argv, options)) {
        print_usage(std::cerr);
        return kExitUsage;
    }

    auto solver = sudoku::make_solver(options.solver);
    if (!solver) {
        std::cerr << "sudoku: unknown solver '" << options.solver << "'\n";
        return kExitUsage;
    }

    std::vector<sudoku::Grid> puzzles;
    if (options.interactive) {
        auto grid = read_interactive();
        if (!grid) {
            std::cerr << "sudoku: input ended before the puzzle was complete\n";
            return kExitUsage;
        }
        puzzles.push_back(*grid);
    } else {
        std::optional<std::vector<sudoku::Grid>> parsed;
        if (options.path.empty() || options.path == "-") {
            parsed = sudoku::read_puzzles(std::cin);
        } else {
            std::ifstream file(options.path);
            if (!file) {
                std::cerr << "sudoku: cannot open '" << options.path << "'\n";
                return kExitUsage;
            }
            parsed = sudoku::read_puzzles(file);
        }
        if (!parsed || parsed->empty()) {
            std::cerr << "sudoku: input is not a valid puzzle (expected 81 cells of 0-9 or '.')\n";
            return kExitUsage;
        }
        puzzles = std::move(*parsed);
    }

    const bool pretty = options.format == Format::kPretty ||
                        (options.format == Format::kAuto && puzzles.size() == 1);
    int exit_code = kExitSolved;

    for (std::size_t i = 0; i < puzzles.size(); ++i) {
        sudoku::Grid grid = puzzles[i];
        sudoku::SolveStats stats;
        const auto start = std::chrono::steady_clock::now();
        const bool solved = solver->solve(grid, &stats);
        const auto elapsed = std::chrono::steady_clock::now() - start;

        if (pretty && i > 0) {
            std::cout << '\n';
        }
        if (solved) {
            std::cout << (pretty ? grid.to_pretty() : grid.to_line() + "\n");
        } else {
            std::cout << "No solution\n";
            exit_code = kExitUnsolvable;
        }

        if (options.stats) {
            const auto micros =
                std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
            std::cerr << "puzzle " << i + 1 << ": solver=" << solver->name()
                      << " nodes=" << stats.nodes << " time_us=" << micros << '\n';
        }
    }
    return exit_code;
}
