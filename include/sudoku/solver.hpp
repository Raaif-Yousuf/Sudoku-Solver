#pragma once

#include <cstddef>
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

/// A solver built on per-row/column/box candidate bitmasks. Propagates naked
/// and hidden singles to a fixed point, then searches depth-first, branching
/// on the empty cell with the fewest remaining candidates (MRV).
class FastSolver final : public Solver {
  public:
    [[nodiscard]] std::string_view name() const override { return "fast"; }
    bool solve(Grid& grid, SolveStats* stats = nullptr) override;
};

/// Counts distinct solutions of `puzzle` using the same bitmask engine as
/// FastSolver, stopping once `limit` solutions have been found. Returns 0 if
/// the puzzle is inconsistent.
[[nodiscard]] std::size_t count_solutions(const Grid& puzzle, std::size_t limit = 2);

/// Creates a solver by name, or returns nullptr for an unknown name.
[[nodiscard]] std::unique_ptr<Solver> make_solver(std::string_view name);

/// Names accepted by make_solver(), in a stable order.
[[nodiscard]] std::vector<std::string_view> solver_names();

}  // namespace sudoku
