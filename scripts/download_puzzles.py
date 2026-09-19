#!/usr/bin/env python3
"""Downloads the Sudoku puzzle sets used by the benchmark tool.

Fetches Peter Norvig's three public puzzle sets (from "Solving Every Sudoku
Puzzle", https://norvig.com/sudoku.html) into bench/data/, verifying each raw
download against a SHA256 hash recorded below (computed the first time this
script downloaded each file). easy50.txt ships in a multi-line grid format
separated by "========" lines; this script converts it to one 81-character
puzzle per line before saving, alongside the untouched top95.txt and
hardest.txt, which are already one puzzle per line.

Uses only the Python 3 standard library.
"""

import hashlib
import pathlib
import sys
import urllib.error
import urllib.request

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
DATA_DIR = REPO_ROOT / "bench" / "data"

# SHA256 of the raw bytes downloaded from each URL, computed the first time
# this script fetched them (2026-09-19). If Norvig ever edits these files,
# re-run with --update-hashes to record the new hash after checking the diff.
PUZZLE_SETS = [
    {
        "name": "easy50",
        "url": "https://norvig.com/easy50.txt",
        "sha256": "42230de8d1ff15d1e2e076bf33718422cd970bf094252da21c11f57088c2ed1d",
        "multiline": True,
    },
    {
        "name": "top95",
        "url": "https://norvig.com/top95.txt",
        "sha256": "b16eff35fbe41fb7b122f21c5c6ba30b85861b47946d33257070df1a69cddd36",
        "multiline": False,
    },
    {
        "name": "hardest",
        "url": "https://norvig.com/hardest.txt",
        "sha256": "398e1e5df4f50723078e3b510e00ea7eb9e104c521a9a186b4c20cd066c0459f",
        "multiline": False,
    },
]

CELL_CHARS = set("0123456789.")


def fetch(url: str) -> bytes:
    request = urllib.request.Request(url, headers={"User-Agent": "sudoku-bench/1.0"})
    with urllib.request.urlopen(request, timeout=30) as response:
        return response.read()


def convert_multiline(raw_text: str) -> list:
    """Converts Norvig's easy50-style grid format to one 81-char line per puzzle.

    Puzzles are 9 lines of 9 digits (0 for blank), separated by lines made up
    entirely of '=' characters. Blank lines are ignored.
    """
    puzzles = []
    current = []
    for line in raw_text.splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        if set(stripped) <= {"="}:
            if current:
                puzzles.append("".join(current))
                current = []
            continue
        current.append(stripped)
        if len(current) == 9:
            puzzles.append("".join(current))
            current = []
    if current:
        puzzles.append("".join(current))
    for puzzle in puzzles:
        if len(puzzle) != 81 or not set(puzzle) <= CELL_CHARS:
            raise ValueError(f"malformed puzzle after conversion: {puzzle!r}")
    return puzzles


def convert_one_per_line(raw_text: str) -> list:
    puzzles = [line.strip() for line in raw_text.splitlines() if line.strip()]
    for puzzle in puzzles:
        if len(puzzle) != 81 or not set(puzzle) <= CELL_CHARS:
            raise ValueError(f"malformed puzzle line: {puzzle!r}")
    return puzzles


def main() -> int:
    DATA_DIR.mkdir(parents=True, exist_ok=True)

    failures = 0
    for entry in PUZZLE_SETS:
        name = entry["name"]
        url = entry["url"]
        expected_hash = entry["sha256"]

        print(f"Downloading {name} from {url} ...")
        try:
            raw = fetch(url)
        except (urllib.error.URLError, OSError) as exc:
            print(f"  ERROR: could not download {url}: {exc}", file=sys.stderr)
            failures += 1
            continue

        actual_hash = hashlib.sha256(raw).hexdigest()
        if actual_hash != expected_hash:
            print(
                f"  ERROR: SHA256 mismatch for {name}\n"
                f"    expected {expected_hash}\n"
                f"    actual   {actual_hash}\n"
                "    Refusing to save. If norvig.com legitimately changed this file, "
                "verify the new content by hand and update PUZZLE_SETS in this script.",
                file=sys.stderr,
            )
            failures += 1
            continue

        raw_text = raw.decode("ascii")
        try:
            if entry["multiline"]:
                puzzles = convert_multiline(raw_text)
            else:
                puzzles = convert_one_per_line(raw_text)
        except ValueError as exc:
            print(f"  ERROR: {exc}", file=sys.stderr)
            failures += 1
            continue

        out_path = DATA_DIR / f"{name}.txt"
        out_path.write_text("\n".join(puzzles) + "\n", encoding="ascii")
        print(f"  OK: saved {len(puzzles)} puzzles to {out_path}")

    if failures:
        print(f"\n{failures} puzzle set(s) failed; see errors above.", file=sys.stderr)
        return 1

    print("\nAll puzzle sets downloaded and verified.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
