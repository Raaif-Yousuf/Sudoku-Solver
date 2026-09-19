# Sudoku Solver

[![CI](https://github.com/Raaif-Yousuf/Sudoku-Solver/actions/workflows/ci.yml/badge.svg)](https://github.com/Raaif-Yousuf/Sudoku-Solver/actions/workflows/ci.yml)

A C++17 Sudoku solver. I started with a plain backtracking solver, then wrote a second one using bitmasks and constraint propagation to see how much faster it could get. On Norvig's 95 hard puzzles it went from 36.9 s to 78 ms.

```text
$ echo "8..........36......7..9.2...5...7.......457.....1...3...1....68..85...1..9....4.." | sudoku --solver fast --stats
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
```

## How it works

The backtracking solver fills cells left to right, tries 1 to 9, and backs up on a clash. The fast solver changes three things:

- Each row, column and box keeps a 9-bit mask of used digits, so a cell's candidates are one bit operation.
- Before branching it fills every cell that has only one option, and every digit that fits in only one place.
- When it has to guess, it picks the cell with the fewest candidates.

`--unique` counts solutions to check that a puzzle has exactly one.

## Results

Mean of 3 runs on my laptop (Core Ultra 7 255H, GCC -O3). Puzzle sets are from [Norvig's essay](https://norvig.com/sudoku.html).

| Puzzle set | Backtracking | Fast | Placements tried (avg) |
|---|---|---|---|
| easy50 | 190 ms | 3 ms | 24,493 → 0.6 |
| top95 | 36.9 s | 78 ms | 4,138,390 → 64.5 |
| hardest | 173 ms | 2 ms | 95,454 → 9 |

It isn't always faster. Norvig's `hard1` has many solutions, so propagation rarely prunes anything, and the fast solver takes 4.1 s where backtracking takes 29 ms.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/sudoku puzzle.txt
ctest --test-dir build -C Release
```

Needs CMake 3.16+ and a C++17 compiler. Puzzles can be 81-character lines (`.` or `0` for blanks) or 9 rows of digits. To rerun the benchmark, `python scripts/download_puzzles.py` then `./build/bench/sudoku_bench bench/data/*.txt`.

The 79 GoogleTest tests run on Linux, Windows and macOS for every pull request, plus a sanitizer build.

## License

MIT
