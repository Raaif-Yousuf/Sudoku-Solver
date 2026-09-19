# Benchmark puzzle sets

These files are downloaded by `scripts/download_puzzles.py` and are not
checked into the repository (see `.gitignore`); this README is the one
tracked file in this directory.

All three sets come from Peter Norvig's essay "Solving Every Sudoku Puzzle"
(<https://norvig.com/sudoku.html>), which describes each set's difficulty.

| File | Source URL | Puzzles |
|---|---|---|
| easy50.txt | https://norvig.com/easy50.txt | 50 |
| top95.txt | https://norvig.com/top95.txt | 95 |
| hardest.txt | https://norvig.com/hardest.txt | 11 |

Puzzle counts above were counted directly from the downloaded files after
conversion (one line per puzzle), not taken from the file names or the essay.

`easy50.txt` is published as a multi-line grid format, 9 lines of 9 digits
per puzzle separated by `========` lines; `download_puzzles.py` converts it
to one 81-character line per puzzle (digits, `0` for blanks) before saving,
the same format `top95.txt` and `hardest.txt` already use. The SHA256 hashes
checked by the download script are of the raw, unconverted download.
