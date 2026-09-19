# Sudoku Solver

[![CI](https://github.com/Raaif-Yousuf/Sudoku-Solver/actions/workflows/ci.yml/badge.svg)](https://github.com/Raaif-Yousuf/Sudoku-Solver/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

A C++17 Sudoku solver library and command-line tool. It has two solvers: the plain backtracking solver I started with, and a faster one that uses bitmask candidate sets, constraint propagation, and fewest-candidates-first search.

## Features

- **Two solvers behind one interface.** `backtracking` is the baseline; `fast` solves Norvig's `top95` hard set about 470x faster (see [Benchmarks](#benchmarks)).
- **Uniqueness check.** `--unique` counts solutions (stopping at 2) to tell whether a puzzle is well-posed.
- **Flexible input.** Reads 81-character lines (`0` or `.` for blanks), 9 rows of digits, or the tool's own pretty output, from a file or stdin. Multi-puzzle files are solved one puzzle per line.
- **Input validation.** Puzzles whose givens already break a rule are rejected instead of "solved".
- **Tested and checked in CI.** 79 GoogleTest and CLI tests run on Linux, Windows (MSVC), and macOS, plus an AddressSanitizer/UBSan build and a clang-format check.

## Demo

Arto Inkala's puzzle, sometimes called the world's hardest Sudoku:

```text
$ echo "8..........36......7..9.2...5...7.......457.....1...3...1....68..85...1..9....4.." \
    | sudoku --solver fast --stats --unique
8 1 2 | 7 5 3 | 6 4 9
9 4 3 | 6 8 2 | 1 7 5
6 7 5 | 4 9 1 | 2 8 3
------+-------+------
1 5 4 | 2 3 7 | 8 9 6
3 6 9 | 8 4 5 | 7 2 1
2 8 7 | 1 6 9 | 5 3 4
------+-------+------
5 2 1 | 9 7 4 | 3 6 8
4 3 8 | 5 2 6 | 9 1 7
7 9 6 | 3 1 8 | 4 5 2
puzzle 1: solver=fast nodes=172 time_us=1724
puzzle 1: unique solution
```

The backtracking solver finds the same solution after 49,558 placements; the fast solver needs 172.

Several puzzles, one per line:

```text
$ head -3 bench/data/top95.txt | sudoku --solver fast
417369825632158947958724316825437169791586432346912758289643571573291684164875293
527316489896542731314987562172453896689271354453698217941825673765134928238769145
617459823248736915539128467982564371374291586156873294823647159791385642465912738
```

## How it works

### Backtracking (`src/backtracking_solver.cpp`)

This is the original algorithm. It visits cells in row-major order, tries digits 1 to 9 in each empty cell, checks the row, column, and 3x3 box for a clash, and recurses. When it reaches a dead end it clears the cell and backtracks. It is simple, but it may try millions of placements on hard puzzles.

### Fast solver (`src/fast_solver.cpp`)

1. **Bitmask candidates.** Each row, column, and box keeps a 9-bit mask of the digits it already uses. A cell's candidates are `~(row | col | box) & 0x1FF`, so each lookup is a few bit operations instead of a scan.
2. **Constraint propagation.** Before each branch the solver repeatedly fills *naked singles* (cells with one candidate) and *hidden singles* (digits that fit in only one cell of a row, column, or box). It stops early when a cell or unit has no options left.
3. **MRV branching.** When propagation stalls, it branches on the empty cell with the fewest candidates (minimum remaining values).
4. **Copy-on-branch state.** The search state is 135 bytes of fixed-size arrays. Each branch copies it instead of undoing moves, so the search never allocates on the heap.

`count_solutions(puzzle, limit)` reuses the same engine and stops once it finds `limit` solutions. The CLI's `--unique` flag uses it.

### Layout

```text
include/sudoku/   public headers: Grid, Solver interface, factory
src/              grid parsing/validation, both solvers, solver factory
app/              command-line tool (sudoku)
tests/            GoogleTest suites, CLI tests, fixture puzzles
bench/            benchmark tool (sudoku_bench) and dataset notes
scripts/          puzzle set download script
```

New solvers implement `sudoku::Solver` and register in `src/solver_factory.cpp`. The parameterized test suite and the benchmark then pick them up automatically.

## Build and run

Requires CMake 3.16+ and a C++17 compiler (tested with GCC, Clang, and MSVC in CI). GoogleTest v1.15.2 is fetched automatically, pinned by SHA256.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

./build/sudoku puzzle.txt                 # solve a file (pretty output)
./build/sudoku --solver fast < puzzles.txt # one solution per line
./build/sudoku --interactive              # original row-by-row prompt
./build/sudoku --help
```

Exit status is `0` if every puzzle was solved, `1` if any had no solution, and `2` for invalid input or usage.

## Tests

```bash
ctest --test-dir build --output-on-failure -C Release
```

The suite has 79 tests. It covers parsing and validation, every registered solver on easy, hard, 17-clue, empty, already-solved, inconsistent, and unsolvable puzzles, `count_solutions`, and CLI exit codes and output. CI also runs it under AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DSUDOKU_SANITIZE=ON
```

## Benchmarks

The puzzle sets come from Peter Norvig's essay [Solving Every Sudoku Puzzle](https://norvig.com/sudoku.html). See [bench/data/README.md](bench/data/README.md) for sources.

```bash
python scripts/download_puzzles.py    # fetches and hash-checks the sets into bench/data/
./build/bench/sudoku_bench --repeat 3 --timeout-ms 120000 \
    bench/data/easy50.txt bench/data/top95.txt bench/data/hardest.txt
```

I ran this on my laptop (Intel Core Ultra 7 255H, Windows 11, GCC 16.1 `-O3`, single thread, mean of 3 runs per puzzle). Every solution was checked against the puzzle:

| Dataset | Puzzles | Solver | Total ms | Mean us/puzzle | Median us | Max us | Mean nodes |
|---|---|---|---|---|---|---|---|
| easy50 | 50 | backtracking | 190.20 | 3803.92 | 556.17 | 33451.60 | 24493.2 |
| easy50 | 50 | fast | 3.18 | 63.50 | 24.22 | 603.10 | 0.6 |
| top95 | 95 | backtracking | 36854.20 | 387938.91 | 26538.10 | 11416212.83 | 4138389.9 |
| top95 | 95 | fast | 78.09 | 821.98 | 429.83 | 4188.67 | 64.5 |
| hardest | 11 | backtracking | 173.13 | 15738.92 | 10686.47 | 42574.83 | 95454.2 |
| hardest | 11 | fast | 2.06 | 187.63 | 145.20 | 600.87 | 9.0 |

"Nodes" counts the digit placements each solver tries while searching. Those counts are deterministic. Wall-clock times varied between runs on my machine: the fastest backtracking run on `easy50` took 78 ms. Treat the timings as ballpark and the node counts as exact. On `top95` the fast solver is about 470x faster and tries about 64,000x fewer placements.

The search strategy has a weakness. Norvig's `hard1` puzzle (in `tests/test_solvers.cpp`) has many solutions and gives constraint propagation little to work with. On that puzzle the fast solver took 4.1 s and 658,725 placements, while the baseline took 29 ms and 419,194 placements. The fast solver's per-node propagation is expensive when it rarely prunes anything.

## License

[MIT](LICENSE) © Raaif Yousuf
