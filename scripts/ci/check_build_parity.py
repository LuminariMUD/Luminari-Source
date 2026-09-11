#!/usr/bin/env python3
"""Fail when the Autotools and CMake source manifests drift apart.

Both build systems keep hand-maintained file lists. This check parses the
variables that must stay equivalent, then reports entries that are missing
from one side, duplicated within a list, or that name files absent from the
tree (or untracked, when run inside a Git checkout).

Usage: scripts/ci/check_build_parity.py [--root DIR]
Exit status is 0 when every manifest pair matches, 1 otherwise.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path

# (Makefile.am variable, CMakeLists.txt variable)
MANIFEST_PAIRS = (
    ("luminari_SOURCES", "SRC_C_FILES"),
    ("cutest_test_files", "CUTEST_TEST_SOURCES"),
    ("world_tool_sources", "WORLD_TOOL_SOURCES"),
    ("world_tool_support_files", "WORLD_TOOL_SUPPORT_FILES"),
    ("help_sync_sources", "HELP_SYNC_SOURCES"),
)

# The production-linked suite must compile the harness, every test file, and
# every production source on both sides. Autotools lists them in
# cutest_SOURCES (via $(luminari_SOURCES)); CMake lists them in the cutest
# add_executable() call (via ${SRC_FILES}). The generated AllTests.c registry
# is checked separately: Autotools carries it in nodist_cutest_SOURCES and
# CMake writes it into the build directory.
CUTEST_HARNESS = "unittests/CuTest/CuTest.c"
CUTEST_REGISTRY = "unittests/CuTest/AllTests.c"

MAKE_REFERENCE = re.compile(r"^\$\(([A-Za-z_][A-Za-z0-9_]*)\)$")
CMAKE_REFERENCE = re.compile(r"^\$\{([A-Za-z_][A-Za-z0-9_]*)\}$")


def expand(
    variables: dict[str, list[str]],
    tokens: list[str],
    reference: re.Pattern[str],
    seen: frozenset[str] = frozenset(),
) -> list[str]:
    """Replace whole-token variable references with their expanded contents.

    Tokens that still contain an unresolved substitution (generated paths such
    as ${CMAKE_CURRENT_BINARY_DIR}/AllTests.c) are dropped: they never name a
    tracked source and cannot be compared against the other manifest.
    """
    expanded: list[str] = []
    for token in tokens:
        match = reference.match(token)
        if match and match.group(1) in variables and match.group(1) not in seen:
            name = match.group(1)
            expanded += expand(variables, variables[name], reference, seen | {name})
        elif "$" not in token:
            expanded.append(token)
    return expanded


def parse_makefile_am(text: str) -> dict[str, list[str]]:
    """Return variable -> raw tokens for backslash-continued assignments."""
    variables: dict[str, list[str]] = {}
    pattern = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.*)$")
    lines = text.splitlines()
    index = 0
    while index < len(lines):
        match = pattern.match(lines[index])
        if not match:
            index += 1
            continue
        name = match.group(1)
        body = match.group(2)
        while body.rstrip().endswith("\\") and index + 1 < len(lines):
            body = body.rstrip()[:-1] + " "
            index += 1
            body += lines[index]
        variables[name] = body.split()
        index += 1
    return variables


def parse_cmake_lists(text: str) -> dict[str, list[str]]:
    """Return variable -> raw tokens for set(VAR ...) blocks, comments stripped."""
    variables: dict[str, list[str]] = {}
    for match in re.finditer(
        r"^\s*set\(([A-Za-z_][A-Za-z0-9_]*)\s*(.*?)\)\s*$", text, re.S | re.M
    ):
        name = match.group(1)
        body = re.sub(r"#[^\n]*", "", match.group(2))
        variables[name] = body.split()
    return variables


def parse_cmake_executable(text: str, target: str) -> list[str] | None:
    """Return the raw source tokens of add_executable(<target> ...)."""
    match = re.search(
        r"^\s*add_executable\(" + re.escape(target) + r"\s*(.*?)\)\s*$",
        text,
        re.S | re.M,
    )
    if match is None:
        return None
    return re.sub(r"#[^\n]*", "", match.group(1)).split()


def tracked_files(root: Path) -> set[str] | None:
    try:
        output = subprocess.run(
            ["git", "-C", str(root), "ls-files", "-z"],
            check=True,
            capture_output=True,
        ).stdout
    except (OSError, subprocess.CalledProcessError):
        return None
    return {entry.decode() for entry in output.split(b"\0") if entry}


def report_list(label: str, entries: list[str]) -> list[str]:
    return [f"  {label}: {entry}" for entry in sorted(entries)]


def check_pair(
    am_name: str,
    am_entries: list[str],
    cm_name: str,
    cm_entries: list[str],
) -> list[str]:
    problems: list[str] = []
    am_dupes = [name for name, count in Counter(am_entries).items() if count > 1]
    cm_dupes = [name for name, count in Counter(cm_entries).items() if count > 1]
    problems += report_list(f"duplicate in Makefile.am {am_name}", am_dupes)
    problems += report_list(f"duplicate in CMakeLists.txt {cm_name}", cm_dupes)
    am_set, cm_set = set(am_entries), set(cm_entries)
    problems += report_list(
        f"listed in Makefile.am {am_name} but not CMakeLists.txt {cm_name}",
        sorted(am_set - cm_set),
    )
    problems += report_list(
        f"listed in CMakeLists.txt {cm_name} but not Makefile.am {am_name}",
        sorted(cm_set - am_set),
    )
    return problems


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[2],
        help="repository root (default: derived from this script's location)",
    )
    args = parser.parse_args()
    root: Path = args.root

    am_text = (root / "Makefile.am").read_text()
    cm_text = (root / "CMakeLists.txt").read_text()
    am_raw = parse_makefile_am(am_text)
    cm_raw = parse_cmake_lists(cm_text)
    am_vars = {
        name: expand(am_raw, tokens, MAKE_REFERENCE) for name, tokens in am_raw.items()
    }
    cm_vars = {
        name: expand(cm_raw, tokens, CMAKE_REFERENCE) for name, tokens in cm_raw.items()
    }

    problems: list[str] = []
    referenced: set[str] = set()
    for am_name, cm_name in MANIFEST_PAIRS:
        if am_name not in am_vars:
            problems.append(f"  Makefile.am does not define {am_name}")
            continue
        if cm_name not in cm_vars:
            problems.append(f"  CMakeLists.txt does not define {cm_name}")
            continue
        problems += check_pair(am_name, am_vars[am_name], cm_name, cm_vars[cm_name])
        referenced.update(am_vars[am_name])
        referenced.update(cm_vars[cm_name])

    # Production-linked suite composition on both sides, after expansion.
    expected_cutest = (
        set(am_vars.get("cutest_test_files", []))
        | set(am_vars.get("luminari_SOURCES", []))
        | {CUTEST_HARNESS}
    )
    am_cutest = am_vars.get("cutest_SOURCES", [])
    if CUTEST_REGISTRY not in am_vars.get("nodist_cutest_SOURCES", []):
        problems.append(
            f"  Makefile.am nodist_cutest_SOURCES does not list {CUTEST_REGISTRY}"
        )
    am_cutest = [entry for entry in am_cutest if entry != CUTEST_REGISTRY]
    problems += report_list(
        "duplicate in Makefile.am cutest_SOURCES",
        [name for name, count in Counter(am_cutest).items() if count > 1],
    )
    problems += report_list(
        "Makefile.am cutest_SOURCES is missing a production-linked source",
        sorted(expected_cutest - set(am_cutest)),
    )
    problems += report_list(
        "Makefile.am cutest_SOURCES lists a file outside the production-linked set",
        sorted(set(am_cutest) - expected_cutest),
    )

    cm_cutest_raw = parse_cmake_executable(cm_text, "cutest")
    if cm_cutest_raw is None:
        problems.append("  CMakeLists.txt does not define add_executable(cutest ...)")
    else:
        if "${CUTEST_ALL_TESTS_SOURCE}" not in cm_cutest_raw:
            problems.append(
                "  CMakeLists.txt cutest target does not compile ${CUTEST_ALL_TESTS_SOURCE}"
            )
        cm_cutest = expand(cm_raw, cm_cutest_raw, CMAKE_REFERENCE)
        problems += report_list(
            "duplicate in CMakeLists.txt cutest target",
            [name for name, count in Counter(cm_cutest).items() if count > 1],
        )
        problems += report_list(
            "CMakeLists.txt cutest target is missing a production-linked source",
            sorted(expected_cutest - set(cm_cutest)),
        )
        problems += report_list(
            "CMakeLists.txt cutest target lists a file outside the production-linked set",
            sorted(set(cm_cutest) - expected_cutest),
        )

    tracked = tracked_files(root)
    generated = {CUTEST_REGISTRY}
    for entry in sorted(referenced - generated):
        if not (root / entry).exists():
            problems.append(f"  manifest entry does not exist: {entry}")
        elif tracked is not None and entry not in tracked:
            problems.append(f"  manifest entry is not tracked by Git: {entry}")

    if problems:
        print("Build manifest parity check failed:")
        print("\n".join(problems))
        return 1
    pair_names = ", ".join(f"{am}={cm}" for am, cm in MANIFEST_PAIRS)
    print(
        f"Build manifest parity OK ({pair_names}; "
        f"production-linked cutest = {len(expected_cutest)} sources on both sides)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
