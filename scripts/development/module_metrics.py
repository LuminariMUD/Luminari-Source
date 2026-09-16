#!/usr/bin/env python3
"""Rank C translation units by how hard they are to change safely.

Size alone is not a defect: a generated table, a constants list, or a wide
dispatch switch can be long and still be easy to reason about. What makes a
translation unit expensive is a long file that is also churning, widely
depended upon, and reaching into shared mutable state. This tool measures
those separately so the ranking reflects risk rather than line count.

Metrics per file
----------------
lines       Physical lines.
code        Lines that are neither blank nor comment. Block and line comments
            are stripped, and so are string literals, so a table of prose
            still counts as code while its surrounding commentary does not.
data        Share of code lines sitting inside a file-scope initialiser
            (``... = { ... }`` or a brace-initialised array). This is the
            table-heavy signal; a file above DATA_HEAVY_SHARE is labelled an
            exception rather than a decomposition candidate.
churn       Commits touching the file within the history window.
fixes       Commits within the window whose subject looks like a bug or
            security fix. A proxy for defect density, taken from real history
            rather than assigned by hand.
fan_in      Files that include this file's companion header. How much of the
            tree a change to its interface can reach.
fan_out     Distinct headers the preprocessor pulls in for this file. How much
            of the tree can force this file to rebuild.
globals     File-scope definitions with external linkage: non-static function
            definitions plus non-static file-scope variables. The symbol
            surface other modules can bind to.

Score
-----
Each metric is scaled to its own maximum across the scanned set, so the score
answers "how extreme is this file relative to this tree" and not "how does it
compare to some absolute budget". Data-heavy files are scored on their
non-table code so a large table cannot by itself push a file up the ranking.

Usage
-----
    scripts/development/module_metrics.py                  # top 40, markdown
    scripts/development/module_metrics.py --top 0          # every file
    scripts/development/module_metrics.py --min-lines 500
    scripts/development/module_metrics.py --since 24.months

Exit status is 0 unless the tree could not be scanned.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path

# A file whose code is at least this much initialiser data is reported as a
# table-heavy exception rather than ranked as a decomposition candidate.
DATA_HEAVY_SHARE = 0.55

# Subjects that indicate a correction rather than a feature.
FIX_SUBJECT = re.compile(
    r"\b(fix|fixes|fixed|bug|regress\w*|crash|leak|overflow|segfault|"
    r"underflow|corrupt\w*|security|vuln\w*|cve|sanitiz\w*|use-after-free)\b",
    re.IGNORECASE,
)

INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.MULTILINE)

# Weight per metric. Size is deliberately not the dominant term.
WEIGHTS = {
    "code": 1.0,
    "churn": 1.0,
    "fixes": 0.75,
    "fan_in": 0.75,
    "fan_out": 0.5,
    "globals": 0.5,
}


def run(args: list[str], root: Path) -> str:
    """Return stdout for a git command, or an empty string if it fails."""
    try:
        done = subprocess.run(
            args, cwd=root, capture_output=True, text=True, check=False, errors="replace"
        )
    except OSError:
        return ""
    return done.stdout if done.returncode == 0 else ""


def strip_noise(text: str) -> str:
    """Remove comments and string literals, preserving line structure.

    Newlines inside removed spans are kept so line numbering and the blank
    versus non-blank classification stay correct.
    """
    out: list[str] = []
    i, n = 0, len(text)
    while i < n:
        two = text[i : i + 2]
        if two == "/*":
            end = text.find("*/", i + 2)
            end = n if end < 0 else end + 2
            out.append("\n" * text.count("\n", i, end))
            i = end
        elif two == "//":
            end = text.find("\n", i)
            end = n if end < 0 else end
            i = end
        elif text[i] in "\"'":
            quote, j = text[i], i + 1
            while j < n and text[j] != quote:
                j += 2 if text[j] == "\\" else 1
            out.append("\n" * text.count("\n", i, min(j + 1, n)))
            i = min(j + 1, n)
        else:
            out.append(text[i])
            i += 1
    return "".join(out)


def data_lines(stripped: str) -> int:
    """Count code lines inside file-scope brace initialisers.

    Walks brace depth over the comment-free text. A ``{`` that follows an
    ``=`` at depth zero opens an initialiser; everything until its matching
    close is table data. Function bodies open at depth zero without a
    preceding ``=`` and are therefore not counted.
    """
    total = 0
    depth = 0
    in_data = False
    data_depth = 0
    saw_equals = False
    continued = False
    for line in stripped.splitlines():
        # Directives carry their own '=' and braces (function-like macros in
        # particular) and never contribute table data.
        directive = continued or line.lstrip().startswith("#")
        continued = directive and line.rstrip().endswith("\\")
        if directive:
            continue
        counted = False
        for ch in line:
            if ch == "=" and depth == 0:
                saw_equals = True
            elif ch == "{":
                depth += 1
                if not in_data and saw_equals and depth == 1:
                    in_data, data_depth = True, depth
            elif ch == "}":
                if in_data and depth == data_depth:
                    in_data = False
                depth = max(0, depth - 1)
            elif ch == ";" and depth == 0:
                saw_equals = False
            if in_data and not counted and line.strip():
                total += 1
                counted = True
    return total


def external_symbols(stripped: str) -> int:
    """Count file-scope definitions that other translation units can bind to."""
    count = 0
    for line in stripped.splitlines():
        if not line or line[0].isspace() or line[0] in "#}/*":
            continue
        if line.lstrip().startswith(("static", "typedef", "extern", "struct ", "union ", "enum ")):
            continue
        if "(" in line and ")" in line and line.rstrip().endswith((";", ",")):
            continue  # a prototype, not a definition
        if re.match(r"^[A-Za-z_][\w \t\*]*\b[A-Za-z_]\w*\s*[\(\[=]", line):
            count += 1
    return count


def collect(root: Path, since: str, min_lines: int) -> list[dict]:
    tracked = run(["git", "ls-files", "*.c", "*.h"], root).split()
    sources = [p for p in tracked if p.endswith(".c") and p.startswith("src/")]
    if not sources:
        return []

    # Fan-in: how many files include each header.
    includers: Counter[str] = Counter()
    for rel in tracked:
        try:
            text = (root / rel).read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        for target in INCLUDE.findall(text):
            includers[Path(target).name] += 1

    # Churn and fix-churn in one pass over the log.
    churn: Counter[str] = Counter()
    fixes: Counter[str] = Counter()
    log = run(
        ["git", "log", f"--since={since}", "--name-status", "-M", "--pretty=format:%x01%s"],
        root,
    )
    # git log walks newest first, so by the time an old path appears its
    # successor is already known and the chain resolves in one pass.
    alias: dict[str, str] = {}

    def current(path: str) -> str:
        seen = set()
        while path in alias and path not in seen:
            seen.add(path)
            path = alias[path]
        return path

    subject = ""
    for line in log.splitlines():
        if line.startswith("\x01"):
            subject = line[1:]
            continue
        parts = line.split("\t")
        if len(parts) < 2:
            continue
        if parts[0].startswith("R") and len(parts) >= 3:
            alias[parts[1]] = current(parts[2])
            touched = current(parts[2])
        else:
            touched = current(parts[1])
        churn[touched] += 1
        if FIX_SUBJECT.search(subject):
            fixes[touched] += 1

    rows: list[dict] = []
    for rel in sources:
        try:
            text = (root / rel).read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        lines = text.count("\n")
        if lines < min_lines:
            continue
        stripped = strip_noise(text)
        code = sum(1 for line in stripped.splitlines() if line.strip())
        data = data_lines(stripped)
        header = Path(rel).with_suffix(".h").name
        rows.append(
            {
                "path": rel,
                "lines": lines,
                "code": code,
                "data": data,
                "share": (data / code) if code else 0.0,
                "churn": churn.get(rel, 0),
                "fixes": fixes.get(rel, 0),
                "fan_in": includers.get(header, 0),
                "fan_out": len(set(INCLUDE.findall(text))),
                "globals": external_symbols(stripped),
            }
        )
    return rows


def score(rows: list[dict]) -> None:
    """Attach a 0-100 score, normalised against the scanned set."""
    # Table data is excluded from the size term so a long table cannot
    # dominate the ranking on its own.
    for row in rows:
        row["logic"] = row["code"] - row["data"]
    peaks = {
        key: max((row["logic"] if key == "code" else row[key]) for row in rows) or 1
        for key in WEIGHTS
    }
    total_weight = sum(WEIGHTS.values())
    for row in rows:
        acc = 0.0
        for key, weight in WEIGHTS.items():
            value = row["logic"] if key == "code" else row[key]
            acc += weight * (value / peaks[key])
        row["score"] = round(100.0 * acc / total_weight, 1)
        row["table_heavy"] = row["share"] >= DATA_HEAVY_SHARE


def emit(rows: list[dict], top: int) -> None:
    ranked = sorted(rows, key=lambda r: r["score"], reverse=True)
    shown = ranked if top <= 0 else ranked[:top]
    print(
        "| # | File | Lines | Code | Table% | Churn | Fixes | Fan-in | Fan-out | Globals | Score |"
    )
    print("| -- | -- | --: | --: | --: | --: | --: | --: | --: | --: | --: |")
    for index, row in enumerate(shown, start=1):
        label = f"`{row['path']}`" + (" _(table)_" if row["table_heavy"] else "")
        print(
            f"| {index} | {label} | {row['lines']} | {row['code']} | "
            f"{100.0 * row['share']:.0f}% | {row['churn']} | {row['fixes']} | "
            f"{row['fan_in']} | {row['fan_out']} | {row['globals']} | {row['score']} |"
        )
    heavy = [r for r in ranked if r["table_heavy"]]
    print()
    print(
        f"Scanned {len(rows)} translation units; "
        f"{len(heavy)} are table-heavy (>= {100 * DATA_HEAVY_SHARE:.0f}% initialiser data)."
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--root", default=".", help="repository root (default: .)")
    parser.add_argument("--top", type=int, default=40, help="rows to print; 0 for all")
    parser.add_argument("--min-lines", type=int, default=400, help="ignore files below this size")
    parser.add_argument("--since", default="18.months", help="git history window")
    args = parser.parse_args()

    root = Path(args.root).resolve()
    rows = collect(root, args.since, args.min_lines)
    if not rows:
        print(f"no tracked C sources found under {root}/src", file=sys.stderr)
        return 1
    score(rows)
    emit(rows, args.top)
    return 0


if __name__ == "__main__":
    sys.exit(main())
