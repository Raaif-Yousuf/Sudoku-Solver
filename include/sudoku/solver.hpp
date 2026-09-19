#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include "sudoku/grid.hpp"

namespace sudoku {

/// Counters a solver fills in while searching. Useful for comparing strategies.
struct SolveStats {
    std::uint64_t nodes = 0;  ///< Number of tentative digit placements tried.
};

/// Common interface for every solving strategy.
class Solver {
public:
    virtual ~Solver() = default;

    /// Short identifier used by the CLI and benchmarks (e.g. "backtracking").
    [[nodiscard]] virtual std::string_view name() const = 0;

    /// Solves `grid` in place. Returns false, leaving `grid` unchanged, if the
    /// puzzle is inconsistent or has no solution. `stats` may be null.
    virtual bool solve(Grid& grid, SolveStats* stats = nullptr) = 0;
};

/// The original recursive backtracking solver: fills cells in row-major order,
/// trying digits 1-9 and checking row, column, and box each time.
class BacktrackingSolver final : public Solver {
public:
    [[nodiscard]] std::string_view name() const override { return "backtracking"; }
    bool solve(Grid& grid, SolveStats* stats = nullptr) override;
};

/// Creates a solver by name, or returns nullptr for an unknown name.
[[nodiscard]] std::unique_ptr<Solver> make_solver(std::string_view name);

/// Names accepted by make_solver(), in a stable order.
[[nodiscard]] std::vector<std::string_view> solver_names();

}  // namespace sudoku
