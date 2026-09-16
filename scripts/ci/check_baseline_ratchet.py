#!/usr/bin/env python3
"""Refuse a change that raises one of the tracked baselines.

Every ratchet under scripts/ci/ reads its baseline from the working tree and
trusts it, so each one holds the tree only at the numbers committed alongside
it. Raising those numbers in the same change that adds the findings is green in
every one of them. This check closes that by reading each baseline from the
diff base as well and failing when an entry grew, when a new entry appeared, or
when an exception was added to a list.

It covers the clang-tidy findings baseline and its unsafe-call site list, the
header self-containment list, the migration and analyzer warning budgets, and
the formatted-SQL baseline. Files renamed by the change are followed, so moving
a source and renaming its baseline entries in the same commit is not a
regression.

The same source can legitimately produce more findings only when what produces
them changes: the tool pin, its configuration, the warning flags, or the
detector. A baseline may therefore grow in a change that modifies one of its
producers, as adopting a new clang-tidy release requires; the growth is still
printed in full for review.

Usage:
  check_baseline_ratchet.py --base REF
  check_baseline_ratchet.py --self-test
"""

import argparse
import fnmatch
import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]


def parse_trailing_count(text):
    """Lines of 'key... count', as in 'src/a.c bugprone-branch-clone 3'."""
    counts = {}
    for raw in text.splitlines():
        raw = raw.strip()
        if not raw or raw.startswith("#"):
            continue
        fields = raw.split()
        counts[tuple(fields[:-1])] = int(fields[-1])
    return counts


def parse_site_counts(text):
    """Lines of 'path count call', summed per path: the recorded sites in a file."""
    counts = {}
    for raw in text.splitlines():
        raw = raw.strip()
        if not raw or raw.startswith("#"):
            continue
        path, count, _call = raw.split(None, 2)
        counts[(path,)] = counts.get((path,), 0) + int(count)
    return counts


def parse_entries(text):
    """Lines that are each one listed exception, as in the header baseline."""
    counts = {}
    for raw in text.splitlines():
        raw = raw.strip()
        if raw and not raw.startswith("#"):
            counts[(raw,)] = 1
    return counts


CLANG_TIDY_PRODUCERS = (".clang-tidy", "scripts/ci/clang-tidy-requirements.txt")
# The warning tier flag lists, the budget reader, and where the compilers are pinned.
WARNING_PRODUCERS = (
    "scripts/deployment/production_profile.sh",
    "scripts/ci/check_warning_budget.py",
    "scripts/ci/local/Dockerfile.gcc-16.2",
    ".github/workflows/test.yml",
    ".github/workflows/toolchain-analysis.yml",
)

# Every tracked baseline, as (glob of repository-relative paths, parser,
# the paths whose change lets it grow).
BASELINES = (
    ("scripts/ci/clang_tidy_baseline.txt", parse_trailing_count, CLANG_TIDY_PRODUCERS),
    ("scripts/ci/clang_tidy_unsafe_sites.txt", parse_site_counts, CLANG_TIDY_PRODUCERS),
    (
        "scripts/ci/header_self_containment_baseline.txt",
        parse_entries,
        ("scripts/ci/check_header_self_containment.py",),
    ),
    (
        "scripts/ci/sql_interpolation_baseline.txt",
        parse_trailing_count,
        ("scripts/ci/check_sql_interpolation.py",),
    ),
    ("scripts/ci/warning_budget_*.txt", parse_trailing_count, WARNING_PRODUCERS),
)


def git(*arguments):
    return subprocess.run(["git", "-C", str(REPO_ROOT), *arguments], capture_output=True, text=True)


def baseline_paths():
    """The files matching BASELINES, as (path, parser, producers).

    Tracked files in a checkout. An exported archive has no Git metadata, and
    make test runs this self-test there too, so its files come from the tree.
    """
    tracked = git("ls-files", "--", "scripts/ci").stdout.split()
    if not tracked:
        tracked = [
            path.relative_to(REPO_ROOT).as_posix()
            for path in (REPO_ROOT / "scripts" / "ci").iterdir()
            if path.is_file()
        ]
    found = []
    for pattern, parser, producers in BASELINES:
        for path in sorted(name for name in tracked if fnmatch.fnmatch(name, pattern)):
            found.append((path, parser, producers))
    return found


def parse_name_status(text):
    """(changed paths, {old path: new path}) from git diff --name-status -M."""
    changed = set()
    renames = {}
    for line in text.splitlines():
        fields = line.split("\t")
        changed.update(fields[1:])
        if len(fields) == 3 and fields[0].startswith("R"):
            renames[fields[1]] = fields[2]
    return changed, renames


def follow_renames(counts, renames):
    """Rewrite the leading path of each key through the rename map."""
    followed = {}
    for key, count in counts.items():
        if key and key[0] in renames:
            key = (renames[key[0]],) + key[1:]
        followed[key] = followed.get(key, 0) + count
    return followed


def regressions(before, now):
    """Return the entries that grew or appeared, as (key, now, before)."""
    grew = []
    for key in sorted(set(before) | set(now)):
        was = before.get(key, 0)
        has = now.get(key, 0)
        if has > was:
            grew.append((key, has, was))
    return grew


def check(base):
    problems = []
    checked = 0
    allowed = 0
    # Against the working tree, so an uncommitted change is measured as CI will see it.
    changed, renames = parse_name_status(git("diff", "--name-status", "-M", base).stdout)
    for path, parser, producers in baseline_paths():
        result = git("show", f"{base}:{path}")
        if result.returncode != 0:
            print(f"{path}: new in this change; nothing to compare")
            continue
        try:
            before = follow_renames(parser(result.stdout), renames)
            now = parser((REPO_ROOT / path).read_text(encoding="ascii"))
        except ValueError as problem:
            problems.append(f"{path}: cannot be parsed: {problem}")
            continue
        checked += 1
        grown = regressions(before, now)
        reasons = sorted(changed.intersection(producers))
        if grown and reasons:
            print(f"{path}: grows with {', '.join(reasons)}, which this change modifies:")
            for key, has, was in grown:
                print(f"  {' '.join(key)} is {has}, was {was} at {base}")
            allowed += 1
            continue
        for key, has, was in grown:
            problems.append(f"{path}: {' '.join(key)} is {has}, was {was} at {base}")
    print(f"{checked} baselines compared with {base}")
    if not problems:
        if allowed:
            print(f"{allowed} baselines grew, each with a change to what produces its findings")
        else:
            print("no baseline grew")
        return 0
    print("a baseline may only shrink:", file=sys.stderr)
    for line in problems:
        print(f"  {line}", file=sys.stderr)
    print(
        "fix the new findings instead of recording them; a baseline grows only in a change "
        "to what produces its findings, listed in BASELINES in this script",
        file=sys.stderr,
    )
    return 1


def self_test():
    assert parse_trailing_count("# c\n\nsrc/a.c check 3\nsrc/b.c check 1\n") == {
        ("src/a.c", "check"): 3,
        ("src/b.c", "check"): 1,
    }
    assert parse_trailing_count("analyzer-malloc-leak 43\n") == {("analyzer-malloc-leak",): 43}
    assert parse_site_counts('# c\nsrc/a.c 2 sprintf(buf, "%s", n)\nsrc/a.c 1 strcpy(a, b)\n') == {
        ("src/a.c",): 3
    }
    assert parse_entries("# c\nsrc/core/a.h\nsrc/core/b.h\n") == {
        ("src/core/a.h",): 1,
        ("src/core/b.h",): 1,
    }

    before = {("src/a.c", "check"): 3, ("src/b.c", "check"): 2}
    assert regressions(before, {("src/a.c", "check"): 4, ("src/b.c", "check"): 2}) == [
        (("src/a.c", "check"), 4, 3)
    ]
    assert regressions(before, {("src/c.c", "check"): 1, ("src/b.c", "check"): 2}) == [
        (("src/c.c", "check"), 1, 0)
    ]
    assert regressions(before, {("src/a.c", "check"): 1}) == []
    # A source moved in the same change follows to its new entry.
    moved = follow_renames(before, {"src/a.c": "src/moved/a.c"})
    assert regressions(moved, {("src/moved/a.c", "check"): 3, ("src/b.c", "check"): 2}) == []
    changed, renames = parse_name_status(
        "M\t.clang-tidy\nR097\tsrc/a.c\tsrc/moved/a.c\nA\tsrc/new.c\n"
    )
    assert changed == {".clang-tidy", "src/a.c", "src/moved/a.c", "src/new.c"}, changed
    assert renames == {"src/a.c": "src/moved/a.c"}, renames
    assert changed.intersection(CLANG_TIDY_PRODUCERS) == {".clang-tidy"}
    # Two entries that merge into one path are summed before comparing.
    assert follow_renames({("src/a.c",): 2, ("src/b.c",): 1}, {"src/a.c": "src/b.c"}) == {
        ("src/b.c",): 3
    }

    tracked = {path: (parser, producers) for path, parser, producers in baseline_paths()}
    assert "scripts/ci/clang_tidy_baseline.txt" in tracked, tracked
    assert any(re.fullmatch(r"scripts/ci/warning_budget_.*\.txt", name) for name in tracked), (
        tracked
    )
    for path, (parser, producers) in tracked.items():
        parser((REPO_ROOT / path).read_text(encoding="ascii"))
        for producer in producers:
            assert (REPO_ROOT / producer).is_file(), f"{path}: producer {producer} does not exist"
    print("check_baseline_ratchet self-test passed")


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument(
        "--base", metavar="REF", help="the revision this change is measured against"
    )
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    if not args.base:
        parser.error("--base is required")
    if git("rev-parse", "-q", "--verify", f"{args.base}^{{commit}}").returncode != 0:
        print(f"{args.base} is not a commit in this checkout", file=sys.stderr)
        return 2
    return check(args.base)


if __name__ == "__main__":
    sys.exit(main())
