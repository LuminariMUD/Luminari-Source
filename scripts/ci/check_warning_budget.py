#!/usr/bin/env python3
"""Ratchet the migration-tier warning budget.

The migration warning tier (see scripts/deployment/production_profile.sh)
covers families with thousands of pre-existing instances. They are not errors;
instead this check reads a build log produced with that tier and compares the
count of each warning class against a per-compiler baseline. A class may only
shrink: growth or a new class fails the check, and a drop should be followed by
``--update`` so the baseline records the lower number.

Counting is by distinct (file, line, column, class) so that a header included
from many translation units, parallel builds, and ccache replays all yield the
same number. Compiler versions differ in what they report, so the baseline is
keyed by the compiler label the CI job pins (for example ``gcc-16`` or
``clang-22``).

Usage:
  check_warning_budget.py --compiler LABEL --log build.log            # compare
  check_warning_budget.py --compiler LABEL --log build.log --update   # lower the baseline
  check_warning_budget.py --compiler LABEL --log build.log --report   # print counts only
  check_warning_budget.py --compiler LABEL --log build.log --list CLASS      # sites by file
  check_warning_budget.py --compiler LABEL --log build.log --by-token CLASS  # sites by identifier
  check_warning_budget.py --self-test
"""

import argparse
import os
import re
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BASELINE_DIR = os.path.join(REPO_ROOT, "scripts", "ci")

WARNING_PATTERN = re.compile(r"^(?P<site>[^\s:]+:\d+:\d+): warning: .*\[-W(?P<cls>[^\]]+)\]\s*$")
ERROR_PATTERN = re.compile(r"^[^\s:]+:\d+:\d+: error: ")


def baseline_path(compiler):
    return os.path.join(BASELINE_DIR, f"warning_budget_{compiler}.txt")


def collect_sites(lines):
    """Return ({(site, class)}, error_count) for the distinct warning sites in lines."""
    seen = set()
    errors = 0
    for line in lines:
        line = line.rstrip("\n")
        match = WARNING_PATTERN.match(line)
        if match:
            seen.add((match.group("site"), match.group("cls")))
        elif ERROR_PATTERN.match(line):
            errors += 1
    return seen, errors


def count_warnings(lines):
    """Return ({class: count}, error_count) for the distinct warning sites in lines."""
    seen, errors = collect_sites(lines)
    counts = {}
    for _site, cls in seen:
        counts[cls] = counts.get(cls, 0) + 1
    return counts, errors


def list_sites(seen, cls, by_token):
    """Print one class's sites grouped by file, or by the identifier at the column."""
    groups = {}
    for site, site_cls in seen:
        if site_cls != cls:
            continue
        path, line, col = site.rsplit(":", 2)
        if by_token:
            source = ""
            try:
                with open(path, encoding="utf-8", errors="replace") as handle:
                    for number, text in enumerate(handle, 1):
                        if number == int(line):
                            source = text
                            break
            except OSError:
                pass
            fragment = source[int(col) - 1:]
            match = re.match(r"[A-Za-z_][A-Za-z_0-9]*", fragment)
            key = match.group(0) if match else fragment[:12].strip() or "?"
        else:
            key = os.path.relpath(path, REPO_ROOT) if path.startswith(REPO_ROOT) else path
        groups.setdefault(key, []).append(site)
    for key in sorted(groups, key=lambda name: (-len(groups[name]), name)):
        print(f"{len(groups[key]):7d}  {key}")
        if not by_token:
            for site in sorted(groups[key], key=lambda text: int(text.rsplit(":", 2)[1])):
                print(f"           {site}")


def read_baseline(path):
    counts = {}
    if not os.path.exists(path):
        return counts
    with open(path, encoding="ascii") as handle:
        for raw in handle:
            raw = raw.strip()
            if not raw or raw.startswith("#"):
                continue
            cls, count = raw.split()
            counts[cls] = int(count)
    return counts


def write_baseline(path, compiler, counts):
    with open(path, "w", encoding="ascii", newline="\n") as handle:
        handle.write(f"# Migration-tier warning budget for {compiler}.\n")
        handle.write("# Distinct warning sites per class; may only shrink. Regenerate with\n")
        handle.write(f"# scripts/ci/check_warning_budget.py --compiler {compiler} --log LOG --update\n")
        for cls in sorted(counts):
            handle.write(f"{cls} {counts[cls]}\n")


def compare(counts, baseline):
    """Return (failures, improvements) as lists of human-readable lines."""
    failures = []
    improvements = []
    for cls in sorted(set(counts) | set(baseline)):
        now = counts.get(cls, 0)
        allowed = baseline.get(cls)
        if allowed is None:
            failures.append(f"new warning class -W{cls}: {now} (not in baseline)")
        elif now > allowed:
            failures.append(f"-W{cls}: {now} exceeds budget {allowed} (+{now - allowed})")
        elif now < allowed:
            improvements.append(f"-W{cls}: {now} is below budget {allowed}")
    return failures, improvements


def self_test():
    log = """
/src/a.c:10:5: warning: conversion from 'int' to 'char' may change value [-Wconversion]
/src/a.c:10:5: warning: conversion from 'int' to 'char' may change value [-Wconversion]
/src/b.c:3:1: warning: no previous prototype for 'f' [-Wmissing-prototypes]
/src/h.h:7:9: warning: format '%d' expects argument of type 'int' [-Wformat=]
/src/h.h:7:9: warning: format '%d' expects argument of type 'int' [-Wformat=]
/src/c.c:1:1: error: unknown type name 'foo'
[ 10%] Building C object CMakeFiles/luminari.dir/src/a.c.o
""".splitlines()
    counts, errors = count_warnings(log)
    assert counts == {"conversion": 1, "missing-prototypes": 1, "format=": 1}, counts
    assert errors == 1, errors
    failures, improvements = compare(counts, {"conversion": 1, "missing-prototypes": 2, "format=": 1})
    assert failures == [], failures
    assert improvements == ["-Wmissing-prototypes: 1 is below budget 2"], improvements
    failures, _ = compare(counts, {"conversion": 0, "missing-prototypes": 1})
    assert failures == [
        "-Wconversion: 1 exceeds budget 0 (+1)",
        "new warning class -Wformat=: 1 (not in baseline)",
    ], failures
    print("check_warning_budget self-test passed")


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--compiler", help="baseline label, e.g. gcc-16 or clang-22")
    parser.add_argument("--log", help="build log captured with the migration warning tier")
    parser.add_argument("--update", action="store_true", help="lower the baseline; refuses growth")
    parser.add_argument("--report", action="store_true", help="print counts without comparing")
    parser.add_argument("--list", metavar="CLASS", help="print one class's sites grouped by file")
    parser.add_argument("--by-token", metavar="CLASS",
                        help="print one class's sites grouped by the identifier at the column")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    if not args.compiler or not args.log:
        parser.error("--compiler and --log are required")

    with open(args.log, encoding="utf-8", errors="replace") as handle:
        seen, errors = collect_sites(handle)
    if args.list or args.by_token:
        list_sites(seen, args.list or args.by_token, bool(args.by_token))
        return 0
    counts = {}
    for _site, cls in seen:
        counts[cls] = counts.get(cls, 0) + 1
    total = sum(counts.values())
    print(f"{args.compiler}: {total} distinct warning sites in {len(counts)} classes, {errors} errors")
    for cls in sorted(counts, key=lambda name: (-counts[name], name)):
        print(f"  {counts[cls]:7d}  -W{cls}")
    if args.report:
        return 0
    if errors:
        print("the build log contains compiler errors; the count is not trustworthy", file=sys.stderr)
        return 1
    if total == 0:
        print("no warnings found; the log was not produced with the migration tier", file=sys.stderr)
        return 1

    path = baseline_path(args.compiler)
    baseline = read_baseline(path)
    failures, improvements = compare(counts, baseline)
    if args.update:
        if failures and baseline:
            print("refusing to raise the budget:", file=sys.stderr)
            for line in failures:
                print("  " + line, file=sys.stderr)
            return 1
        write_baseline(path, args.compiler, counts)
        print(f"wrote {os.path.relpath(path, REPO_ROOT)}")
        return 0
    if not baseline:
        print(f"no baseline at {os.path.relpath(path, REPO_ROOT)}; create it with --update", file=sys.stderr)
        return 1
    for line in improvements:
        print("improved: " + line)
    if improvements:
        print(f"lower the budget with: scripts/ci/check_warning_budget.py --compiler {args.compiler} --log LOG --update")
    if failures:
        print("warning budget exceeded:", file=sys.stderr)
        for line in failures:
            print("  " + line, file=sys.stderr)
        return 1
    print("warning budget respected")
    return 0


if __name__ == "__main__":
    sys.exit(main())
