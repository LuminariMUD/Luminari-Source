#!/usr/bin/env python3
"""Hold measured coverage to the floors in scripts/ci/coverage_policy.json.

The Coverage job in .github/workflows/test.yml writes a gcovr Cobertura report
of the sources under src/ (coverage.xml). This check reads that report and
fails when:

- repository line or branch coverage is below its floor;
- a critical subsystem's line or branch coverage is below its floor;
- with --base REF, the executable lines a change adds or modifies in a
  subsystem's sources are covered less than that subsystem's line floor, so
  new or changed critical code cannot pull its subsystem down;
- the report names a path that is not a normalized relative path under src/;
- a subsystem pattern matches no source in the report, its owner file does not
  exist, or one of its named tests is not a Test function in the
  production-linked suite (cutest_test_files in Makefile.am).

Floors are measured values, never targets. A result above a floor is printed
as an improvement to record in the policy; scripts/ci/check_baseline_ratchet.py
refuses a change that lowers a floor.

Usage:
  check_coverage.py --report coverage.xml [--base REF]
  check_coverage.py --self-test
"""

import argparse
import fnmatch
import io
import json
import math
import re
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path, PurePosixPath

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[1]
POLICY_PATH = SCRIPT_DIR / "coverage_policy.json"
sys.path.insert(0, str(SCRIPT_DIR))

from check_build_parity import parse_makefile_am  # noqa: E402

CONDITION = re.compile(r"\((\d+)/(\d+)\)")
HUNK = re.compile(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@")
TEST_DEFINITION = re.compile(r"^void (Test\w*)\s*\(\s*CuTest\s*\*\s*\w+\s*\)", re.MULTILINE)
KINDS = ("lines", "branches")


def percent(covered, total):
    return 100.0 * covered / total if total else 100.0


def floor2(value):
    """A measured percentage as a floor: truncated, so the measurement meets it."""
    return math.floor(value * 100.0 + 1e-9) / 100.0


def load_report(source):
    """Read a Cobertura report into ({path: [(line, hits, covered, branches)]}, problems).

    A line can appear more than once in a file, once per function defined on it
    (ACMD expands to two); each entry counts, as in gcovr's own totals.
    """
    files = {}
    problems = []
    for cls in ET.parse(source).getroot().iter("class"):
        path = cls.get("filename", "")
        normalized = PurePosixPath(path)
        if (
            normalized.is_absolute()
            or ".." in normalized.parts
            or str(normalized) != path
            or normalized.parts[:1] != ("src",)
        ):
            problems.append(f"report path is not a normalized path under src/: {path!r}")
            continue
        entries = files.setdefault(path, [])
        # Only the class's own line list: each method repeats its lines.
        for line in cls.findall("lines/line"):
            covered = total = 0
            condition = line.get("condition-coverage")
            if condition:
                match = CONDITION.search(condition)
                covered, total = int(match.group(1)), int(match.group(2))
            entries.append((int(line.get("number")), int(line.get("hits")), covered, total))
    return files, problems


def measure(files, paths):
    """Return {"lines": (covered, total), "branches": (covered, total)} for paths."""
    counts = {"lines": [0, 0], "branches": [0, 0]}
    for path in paths:
        for _number, hits, covered, total in files[path]:
            counts["lines"][0] += 1 if hits > 0 else 0
            counts["lines"][1] += 1
            counts["branches"][0] += covered
            counts["branches"][1] += total
    return {kind: tuple(value) for kind, value in counts.items()}


def subsystem_sources(patterns, files):
    """The report paths a subsystem's patterns match, and the patterns matching none."""
    matched = set()
    unmatched = []
    for pattern in patterns:
        hits = [path for path in files if fnmatch.fnmatchcase(path, pattern)]
        if not hits:
            unmatched.append(pattern)
        matched.update(hits)
    return sorted(matched), unmatched


def parse_changed_lines(diff_text):
    """{path: {new-side line numbers}} added or modified in a unified=0 diff."""
    changed = {}
    path = None
    for line in diff_text.splitlines():
        if line.startswith("+++ "):
            target = line[4:]
            path = target[2:] if target.startswith("b/") else None
            continue
        match = HUNK.match(line)
        if match and path is not None:
            start = int(match.group(1))
            count = int(match.group(2)) if match.group(2) is not None else 1
            changed.setdefault(path, set()).update(range(start, start + count))
    return changed


def changed_lines(base):
    result = subprocess.run(
        ["git", "-C", str(REPO_ROOT), "diff", "--unified=0", "--no-color", "-M", base, "--"],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise SystemExit(f"git diff against {base} failed: {result.stderr.strip()}")
    return parse_changed_lines(result.stdout)


def suite_tests(root):
    """Names of the Test functions compiled into the production-linked suite."""
    variables = parse_makefile_am((root / "Makefile.am").read_text(encoding="utf-8"))
    names = set()
    for source in variables.get("cutest_test_files", []):
        names.update(TEST_DEFINITION.findall((root / source).read_text(encoding="utf-8")))
    return names


def evaluate(files, policy, changed, tests, root):
    """Compare a report with the policy. Returns (report lines, improvements, failures)."""
    report = []
    improvements = []
    failures = []

    def compare(label, measured, floors):
        text = []
        for kind in KINDS:
            covered, total = measured[kind]
            value = percent(covered, total)
            floor = floors[kind]
            text.append(f"{kind} {value:6.2f}% ({covered}/{total}, floor {floor:.2f})")
            if value + 1e-9 < floor:
                failures.append(
                    f"{label} {kind} coverage {value:.2f}% is below its floor {floor:.2f}"
                )
            elif floor2(value) > floor:
                improvements.append(f"{label} {kind} floor can rise to {floor2(value):.2f}")
        report.append(f"{label}: " + ", ".join(text))

    compare("repository", measure(files, sorted(files)), policy["repository"])
    for name, subsystem in sorted(policy["subsystems"].items()):
        sources, unmatched = subsystem_sources(subsystem["sources"], files)
        for pattern in unmatched:
            failures.append(f"{name}: source pattern {pattern} matches nothing in the report")
        if not (root / subsystem["owner"]).is_file():
            failures.append(f"{name}: owner {subsystem['owner']} does not exist")
        for test in subsystem["tests"]:
            if test not in tests:
                failures.append(f"{name}: {test} is not a Test function in the suite")
        if not sources:
            continue
        compare(name, measure(files, sources), subsystem["floor"])
        if changed is None:
            continue
        covered = total = 0
        uncovered = []
        for path in sources:
            executed = {}
            for number, hits, _covered, _total in files[path]:
                executed[number] = executed.get(number, False) or hits > 0
            for number in sorted(changed.get(path, set()).intersection(executed)):
                total += 1
                if executed[number]:
                    covered += 1
                else:
                    uncovered.append(f"{path}:{number}")
        if total == 0:
            continue
        value = percent(covered, total)
        floor = subsystem["floor"]["lines"]
        report.append(f"{name} changed lines: {value:6.2f}% ({covered}/{total}, floor {floor:.2f})")
        if value + 1e-9 < floor:
            failures.append(
                f"{name}: changed lines are {value:.2f}% covered, below its {floor:.2f} line floor;"
                f" not executed by any test: {', '.join(uncovered)}"
            )
    return report, improvements, failures


def self_test():
    xml = """<?xml version="1.0"?>
<coverage><packages><package><classes>
  <class filename="src/a/a.c"><methods><method name="f"><lines>
    <line number="1" hits="2"/></lines></method></methods><lines>
    <line number="1" hits="2" branch="true" condition-coverage="50% (1/2)"/>
    <line number="2" hits="0"/>
    <line number="4" hits="1"/>
    <line number="5" hits="0" branch="true" condition-coverage="0% (0/2)"/>
  </lines></class>
  <class filename="src/b/b.c"><lines><line number="7" hits="3"/></lines></class>
</classes></package></packages></coverage>
"""
    files, problems = load_report(io.StringIO(xml))
    assert problems == [], problems
    assert files["src/a/a.c"][0] == (1, 2, 1, 2), files
    measured = measure(files, ["src/a/a.c", "src/b/b.c"])
    assert measured == {"lines": (3, 5), "branches": (1, 4)}, measured
    assert floor2(percent(2, 3)) == 66.66 and floor2(50.0) == 50.0
    assert subsystem_sources(["src/a/*.c", "src/z/*.c"], files) == (["src/a/a.c"], ["src/z/*.c"])

    diff = (
        "diff --git a/src/a/a.c b/src/a/a.c\n--- a/src/a/a.c\n+++ b/src/a/a.c\n"
        "@@ -1 +1 @@\n-x\n+y\n@@ -3,0 +4,2 @@\n+p\n+q\n@@ -9,2 +10,0 @@\n-r\n-s\n"
        "diff --git a/src/gone.c b/src/gone.c\n--- a/src/gone.c\n+++ /dev/null\n@@ -1 +0,0 @@\n-t\n"
    )
    assert parse_changed_lines(diff) == {"src/a/a.c": {1, 4, 5}}, parse_changed_lines(diff)

    policy = {
        "repository": {"lines": 60.0, "branches": 25.0},
        "subsystems": {
            "alpha": {
                "sources": ["src/a/*.c"],
                "owner": "scripts/ci/check_coverage.py",
                "tests": ["Test_alpha"],
                "floor": {"lines": 50.0, "branches": 25.0},
            }
        },
    }
    report, improvements, failures = evaluate(files, policy, None, {"Test_alpha"}, REPO_ROOT)
    assert failures == [], failures
    assert improvements == [], improvements
    assert report[0].startswith("repository: lines  60.00% (3/5, floor 60.00)"), report

    # Line 1 is covered and lines 4 and 5 were changed: 1 of 2 executable changed lines.
    _, _, failures = evaluate(files, policy, {"src/a/a.c": {1, 5}}, {"Test_alpha"}, REPO_ROOT)
    assert failures == [], failures
    _, _, failures = evaluate(files, policy, {"src/a/a.c": {2, 3, 5}}, {"Test_alpha"}, REPO_ROOT)
    assert len(failures) == 1 and "src/a/a.c:2, src/a/a.c:5" in failures[0], failures

    policy["repository"]["lines"] = 60.01
    policy["subsystems"]["alpha"]["floor"]["branches"] = 20.0
    policy["subsystems"]["alpha"]["owner"] = "missing/owner.c"
    policy["subsystems"]["alpha"]["sources"].append("src/none/*.c")
    _, improvements, failures = evaluate(files, policy, None, set(), REPO_ROOT)
    assert improvements == ["alpha branches floor can rise to 25.00"], improvements
    assert failures == [
        "repository lines coverage 60.00% is below its floor 60.01",
        "alpha: source pattern src/none/*.c matches nothing in the report",
        "alpha: owner missing/owner.c does not exist",
        "alpha: Test_alpha is not a Test function in the suite",
    ], failures

    _, problems = load_report(
        io.StringIO(
            '<coverage><class filename="/abs/src/a.c"><lines/></class>'
            '<class filename="src/../lib/b.c"><lines/></class>'
            '<class filename="unittests/c.c"><lines/></class></coverage>'
        )
    )
    assert len(problems) == 3, problems

    live = json.loads(POLICY_PATH.read_text(encoding="utf-8"))
    tests = suite_tests(REPO_ROOT)
    for name, subsystem in live["subsystems"].items():
        assert (REPO_ROOT / subsystem["owner"]).is_file(), f"{name}: missing owner"
        for test in subsystem["tests"]:
            assert test in tests, f"{name}: {test} is not in the production-linked suite"
        for kind in KINDS:
            assert floor2(subsystem["floor"][kind]) == subsystem["floor"][kind], name
    print("check_coverage self-test passed")


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--report", type=Path, help="gcovr Cobertura XML report")
    parser.add_argument("--base", metavar="REF", help="also check the lines changed since REF")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    if args.report is None:
        parser.error("--report is required")
    policy = json.loads(POLICY_PATH.read_text(encoding="utf-8"))
    files, problems = load_report(args.report)
    changed = changed_lines(args.base) if args.base else None
    report, improvements, failures = evaluate(
        files, policy, changed, suite_tests(REPO_ROOT), REPO_ROOT
    )
    failures = problems + failures
    for line in report:
        print(line)
    for line in improvements:
        print(f"improved: {line}; record it in {POLICY_PATH.relative_to(REPO_ROOT)}")
    if failures:
        print("coverage policy failed:", file=sys.stderr)
        for line in failures:
            print(f"  {line}", file=sys.stderr)
        return 1
    print("coverage policy passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
