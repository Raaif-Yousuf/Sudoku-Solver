#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "sudoku/grid.hpp"
#include "sudoku/solver.hpp"

namespace {

constexpr int kExitOk = 0;
constexpr int kExitFailures = 1;
constexpr int kExitUsage = 2;

struct Options {
    std::vector<std::string> solvers;  // empty means "every registered solver"
    int repeat = 1;
    long long timeout_ms = 0;  // 0 means no per-file-per-solver budget
    std::vector<std::string> files;
};

void print_usage(std::ostream& os) {
    os << "Usage: sudoku_bench [--solver NAME]... [--repeat N] [--timeout-ms MS] FILE...\n"
          "\n"
          "Benchmarks every registered solver (or just the ones named with --solver)\n"
          "against one or more puzzle files, each read with sudoku::read_puzzles.\n"
          "Prints a markdown table with one row per dataset/solver combination.\n"
          "\n"
          "Options:\n"
          "  --solver NAME    solver to benchmark (repeatable; default: every solver\n"
          "                   returned by sudoku::solver_names())\n"
          "  --repeat N       solve each puzzle N times and average the timing, to\n"
          "                   reduce noise on very fast puzzles (default: 1)\n"
          "  --timeout-ms MS  per-file, per-solver wall-clock budget in milliseconds;\n"
          "                   checked between puzzles, not mid-solve; 0 means no limit\n"
          "                   (default: 0)\n"
          "  -h, --help       show this message\n"
          "\n"
          "Exit status: 0 if every puzzle in every dataset solved correctly, 1 if any\n"
          "solved result failed verification, 2 on invalid usage or input.\n";
}

// Returns false (after printing a message) if the arguments are invalid.
bool parse_args(int argc, char** argv, Options& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        auto next_value = [&](std::string& out) {
            if (i + 1 >= argc) {
                std::cerr << "sudoku_bench: " << arg << " needs a value\n";
                return false;
            }
            out = argv[++i];
            return true;
        };

        if (arg == "-h" || arg == "--help") {
            print_usage(std::cout);
            std::exit(kExitOk);
        } else if (arg == "--solver") {
            std::string value;
            if (!next_value(value)) {
                return false;
            }
            options.solvers.push_back(value);
        } else if (arg == "--repeat") {
            std::string value;
            if (!next_value(value)) {
                return false;
            }
            try {
                options.repeat = std::stoi(value);
            } catch (const std::exception&) {
                std::cerr << "sudoku_bench: --repeat must be an integer\n";
                return false;
            }
            if (options.repeat < 1) {
                std::cerr << "sudoku_bench: --repeat must be at least 1\n";
                return false;
            }
        } else if (arg == "--timeout-ms") {
            std::string value;
            if (!next_value(value)) {
                return false;
            }
            try {
                options.timeout_ms = std::stoll(value);
            } catch (const std::exception&) {
                std::cerr << "sudoku_bench: --timeout-ms must be an integer\n";
                return false;
            }
            if (options.timeout_ms < 0) {
                std::cerr << "sudoku_bench: --timeout-ms must not be negative\n";
                return false;
            }
        } else if (arg.size() > 1 && arg[0] == '-') {
            std::cerr << "sudoku_bench: unknown option '" << arg << "'\n";
            return false;
        } else {
            options.files.emplace_back(arg);
        }
    }
    if (options.files.empty()) {
        std::cerr << "sudoku_bench: no puzzle files given\n";
        return false;
    }
    return true;
}

void print_build_info() {
    std::cout << "Build:"
#if defined(__clang__)
              << " Clang " << __clang_version__
#elif defined(__GNUC__)
              << " GCC " << __VERSION__
#elif defined(_MSC_VER)
              << " MSVC " << _MSC_VER
#else
              << " unknown compiler"
#endif
#if defined(NDEBUG)
              << ", NDEBUG defined"
#else
              << ", NDEBUG not defined"
#endif
              << "\n\n";
}

// Strips directory and extension from a path, without pulling in <filesystem>.
std::string dataset_name(const std::string& path) {
    const std::size_t slash = path.find_last_of("/\\");
    const std::string filename = (slash == std::string::npos) ? path : path.substr(slash + 1);
    const std::size_t dot = filename.find_last_of('.');
    return (dot == std::string::npos) ? filename : filename.substr(0, dot);
}

// Result of benchmarking one solver against one file.
struct BenchResult {
    std::size_t completed = 0;
    int failures = 0;
    bool timed_out = false;
    double total_ms = 0.0;
    double mean_us = 0.0;
    double median_us = 0.0;
    double max_us = 0.0;
    double mean_nodes = 0.0;
};

BenchResult run_bench(sudoku::Solver& solver, const std::vector<sudoku::Grid>& puzzles, int repeat,
                      long long timeout_ms) {
    BenchResult result;
    std::vector<double> times_us;
    std::vector<std::uint64_t> nodes;
    times_us.reserve(puzzles.size());
    nodes.reserve(puzzles.size());

    const auto budget_start = std::chrono::steady_clock::now();

    for (const sudoku::Grid& puzzle : puzzles) {
        if (timeout_ms > 0) {
            const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::steady_clock::now() - budget_start)
                                        .count();
            if (elapsed_ms >= timeout_ms) {
                result.timed_out = true;
                break;
            }
        }

        double total_ns = 0.0;
        sudoku::SolveStats last_stats;
        sudoku::Grid last_grid;
        bool solved = false;

        for (int r = 0; r < repeat; ++r) {
            sudoku::Grid work = puzzle;
            sudoku::SolveStats stats;
            const auto t0 = std::chrono::steady_clock::now();
            solved = solver.solve(work, &stats);
            const auto t1 = std::chrono::steady_clock::now();
            total_ns += std::chrono::duration<double, std::nano>(t1 - t0).count();
            last_stats = stats;
            last_grid = work;
        }

        const bool ok = solved && last_grid.is_solved() && last_grid.extends(puzzle);
        if (!ok) {
            ++result.failures;
        }

        times_us.push_back(total_ns / static_cast<double>(repeat) / 1000.0);
        nodes.push_back(last_stats.nodes);
        ++result.completed;
    }

    const double sum_us = std::accumulate(times_us.begin(), times_us.end(), 0.0);
    result.total_ms = sum_us / 1000.0;
    result.mean_us = times_us.empty() ? 0.0 : sum_us / static_cast<double>(times_us.size());

    std::vector<double> sorted_times = times_us;
    std::sort(sorted_times.begin(), sorted_times.end());
    if (!sorted_times.empty()) {
        const std::size_t mid = sorted_times.size() / 2;
        result.median_us = (sorted_times.size() % 2 == 0)
                               ? (sorted_times[mid - 1] + sorted_times[mid]) / 2.0
                               : sorted_times[mid];
        result.max_us = sorted_times.back();
    }

    const std::uint64_t sum_nodes = std::accumulate(nodes.begin(), nodes.end(), std::uint64_t{0});
    result.mean_nodes =
        nodes.empty() ? 0.0 : static_cast<double>(sum_nodes) / static_cast<double>(nodes.size());

    return result;
}

}  // namespace

int main(int argc, char** argv) {
    Options options;
    if (!parse_args(argc, argv, options)) {
        print_usage(std::cerr);
        return kExitUsage;
    }

    std::vector<std::string> solver_names;
    if (options.solvers.empty()) {
        for (std::string_view name : sudoku::solver_names()) {
            solver_names.emplace_back(name);
        }
    } else {
        solver_names = options.solvers;
    }

    print_build_info();
    std::cout << "| Dataset | Puzzles | Solver | Total ms | Mean us/puzzle | Median us | Max us | "
                 "Mean nodes |\n";
    std::cout << "|---|---|---|---|---|---|---|---|\n";

    int total_failures = 0;

    for (const std::string& file_path : options.files) {
        std::ifstream file(file_path);
        if (!file) {
            std::cerr << "sudoku_bench: cannot open '" << file_path << "'\n";
            return kExitUsage;
        }
        std::optional<std::vector<sudoku::Grid>> parsed = sudoku::read_puzzles(file);
        if (!parsed || parsed->empty()) {
            std::cerr << "sudoku_bench: '" << file_path << "' has no valid puzzles\n";
            return kExitUsage;
        }
        const std::vector<sudoku::Grid>& puzzles = *parsed;
        const std::string dataset = dataset_name(file_path);

        for (const std::string& solver_name : solver_names) {
            std::unique_ptr<sudoku::Solver> solver = sudoku::make_solver(solver_name);
            if (!solver) {
                std::cerr << "sudoku_bench: unknown solver '" << solver_name << "'\n";
                return kExitUsage;
            }

            const BenchResult result =
                run_bench(*solver, puzzles, options.repeat, options.timeout_ms);
            total_failures += result.failures;

            std::cout << "| " << dataset << " | " << puzzles.size() << " | " << solver_name
                      << " | ";
            if (result.timed_out) {
                std::cout << "timeout after " << result.completed << " puzzles | | | | |\n";
            } else {
                std::cout << std::fixed << std::setprecision(2) << result.total_ms << " | "
                          << result.mean_us << " | " << result.median_us << " | " << result.max_us
                          << " | " << std::setprecision(1) << result.mean_nodes << " |\n";
            }

            if (result.failures > 0) {
                std::cerr << "sudoku_bench: " << result.failures
                          << " puzzle(s) failed verification for solver '" << solver_name
                          << "' on '" << file_path << "'\n";
            }
        }
    }

    return total_failures > 0 ? kExitFailures : kExitOk;
}
