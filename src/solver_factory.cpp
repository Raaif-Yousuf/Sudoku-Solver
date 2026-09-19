#include "sudoku/solver.hpp"

namespace sudoku {

std::unique_ptr<Solver> make_solver(std::string_view name) {
    if (name == "backtracking") {
        return std::make_unique<BacktrackingSolver>();
    }
    return nullptr;
}

std::vector<std::string_view> solver_names() {
    return {"backtracking"};
}

}  // namespace sudoku
