#!/usr/bin/env python3
"""Ratchet clang-tidy findings against a per-file, per-check baseline.

The checks and their options come from the tracked .clang-tidy, applied by the
clang-tidy release pinned in scripts/ci/clang-tidy-requirements.txt. The
compilation database comes from the canonical CMake configuration
(``cmake --preset analysis``): every production translation unit, the
production-linked test suite, and the utilities. Before analyzing anything the
check proves that the database has a command for every C source named by the
Autotools manifests.

Each source is analyzed once, with the server's compile command when the file
also builds into the test binary. Findings are counted as distinct (file,
line, column, check) sites and compared per (file, check) with
scripts/ci/clang_tidy_baseline.txt. A count may only shrink: growth, or a
check that appears in a file for the first time, fails. A lower count is
reported; record it with --update, which analyzes the whole tree. Without a
baseline file, --update records every finding as the first baseline.

A count cannot see one finding swapped for another in the same file, so every
bugprone-unsafe-functions call, including the unbounded string functions the
configuration adds to it, is held by call text as well, in
scripts/ci/clang_tidy_unsafe_sites.txt. A call
the list does not hold fails even when the file's count is unchanged. Editing
a recorded call changes its text too, so --update re-records the list whenever
no count grew; it never accepts a higher count.

With --base REF only the translation units affected by ``git diff REF`` are
analyzed: changed sources, and every source that includes a changed header.
Counts for a source are compared when it was analyzed; counts for a header
only when every source that includes it was. A change to the analysis
configuration itself analyzes everything.

Inline suppressions must name the checks they silence and give a reason, as
in ``/* NOLINTNEXTLINE(check-name) -- reason */``. A suppression without both,
with a wildcard, or naming a check the pinned release does not have fails.

Usage:
  check_clang_tidy.py --build-dir build/analysis [--base REF] [--report-dir DIR]
  check_clang_tidy.py --build-dir build/analysis --update
  check_clang_tidy.py --self-test
"""

import argparse
import concurrent.futures
import json
import os
import re
import shlex
import subprocess
import sys
import tempfile
import time
from collections import Counter
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[1]
sys.path.insert(0, str(SCRIPT_DIR))

from check_build_parity import MAKE_REFERENCE, expand, parse_makefile_am  # noqa: E402

BASELINE_PATH = SCRIPT_DIR / "clang_tidy_baseline.txt"
SITES_PATH = SCRIPT_DIR / "clang_tidy_unsafe_sites.txt"
REQUIREMENTS_PATH = SCRIPT_DIR / "clang-tidy-requirements.txt"
CONFIG_PATH = REPO_ROOT / ".clang-tidy"

# The check whose sites are held by call text, not only by count: the sprintf,
# vsprintf, strcpy, and strcat entries .clang-tidy adds, and the check's own
# defaults such as rewind.
SITE_TRACKED_CHECK = "bugprone-unsafe-functions"
# A call spanning more lines than this is recorded from what fits.
SITE_TEXT_LINES = 20

# cutest compiles every production source a second time with LUMINARI_CUTEST;
# the server's command is the one analyzed.
TARGET_PREFERENCE = ("luminari", "cutest")
# Autotools manifests whose C sources must each have a database command.
COVERED_MANIFESTS = (("luminari_SOURCES", "luminari"), ("cutest_test_files", "cutest"))
# A change to any of these changes how every unit is analyzed.
FULL_RUN_PATHS = {
    ".clang-tidy",
    "CMakeLists.txt",
    "CMakePresets.json",
    "scripts/ci/check_clang_tidy.py",
    "scripts/ci/clang-tidy-requirements.txt",
    "scripts/ci/clang_tidy_baseline.txt",
    "scripts/ci/clang_tidy_unsafe_sites.txt",
    "scripts/deployment/production_profile.sh",
}
FULL_RUN_PREFIXES = ("cmake/",)
SOURCE_PATHSPECS = ("src/*.[ch]", "unittests/*.[ch]", "util/*.[ch]")

DIAGNOSTIC_PATTERN = re.compile(
    r"^(?P<path>[^\s:][^:]*):(?P<line>\d+):(?P<column>\d+): (?P<severity>warning|error): "
    r"(?P<message>.*?)(?: \[(?P<checks>[^\] ]+)\])?$"
)
SUPPRESSION_PATTERN = re.compile(r"NOLINT(NEXTLINE|BEGIN|END)?")
SUPPRESSION_CHECKS = re.compile(r"\((?P<checks>[^()]*)\)(?P<rest>.*)")
SUPPRESSION_REASON = re.compile(r"\s*--\s*\S")


class EnvironmentProblem(Exception):
    """The analysis cannot run as configured; exit status 2."""


def compile_arguments(entry):
    if "arguments" in entry:
        return list(entry["arguments"])
    return shlex.split(entry["command"])


def repository_path(directory, name, build_dir):
    """Return the repository-relative path of a file, or None outside the source tree."""
    path = Path(os.path.realpath(os.path.join(directory, name)))
    if path.is_relative_to(build_dir) or not path.is_relative_to(REPO_ROOT):
        return None
    return path.relative_to(REPO_ROOT).as_posix()


def target_name(entry):
    match = re.match(r"(?:.*/)?CMakeFiles/([^/]+)\.dir/", entry.get("output", ""))
    return match.group(1) if match else ""


def select_units(database, build_dir):
    """Map each repository source to the one compile command analyzed for it."""
    chosen = {}
    for entry in database:
        path = repository_path(entry["directory"], entry["file"], build_dir)
        if path is None:
            continue
        target = target_name(entry)
        rank = (
            TARGET_PREFERENCE.index(target)
            if target in TARGET_PREFERENCE
            else len(TARGET_PREFERENCE)
        )
        if path not in chosen or rank < chosen[path][0]:
            chosen[path] = (rank, entry)
    return {path: entry for path, (_, entry) in chosen.items()}


def coverage_problems(database, build_dir, makefile_text):
    """Report manifest C sources that have no command for their target in the database."""
    variables = parse_makefile_am(makefile_text)
    by_target = {}
    for entry in database:
        path = repository_path(entry["directory"], entry["file"], build_dir)
        if path is not None:
            by_target.setdefault(target_name(entry), set()).add(path)
    problems = []
    for variable, target in COVERED_MANIFESTS:
        expected = {
            name
            for name in expand(variables, variables.get(variable, []), MAKE_REFERENCE)
            if name.endswith(".c")
        }
        if not expected:
            problems.append(f"Makefile.am {variable} names no C sources")
        for name in sorted(expected - by_target.get(target, set())):
            problems.append(
                f"the compilation database has no {target} command for {name} ({variable})"
            )
    return problems


def database_compiler_problem(database):
    """clang-tidy reads compiler flags from the database; GCC-only flags would be findings."""
    for entry in database:
        driver = os.path.basename(compile_arguments(entry)[0])
        if "clang" not in driver:
            return (
                f"the compilation database was generated with {driver}; "
                "configure it with Clang: cmake --preset analysis"
            )
    return None


def parse_dependencies(text):
    """Return the prerequisites of the rule printed by -MM."""
    _, _, prerequisites = text.replace("\\\n", " ").partition(":")
    return prerequisites.split()


def include_map(units, build_dir, jobs):
    """Return {header: {sources that include it}}, or None when a scan fails."""

    def scan(item):
        path, entry = item
        command = []
        skip = False
        for argument in compile_arguments(entry):
            if skip:
                skip = False
            elif argument == "-o":
                skip = True
            elif argument != "-c":
                command.append(argument)
        result = subprocess.run(
            command + ["-MM"], cwd=entry["directory"], capture_output=True, text=True
        )
        if result.returncode != 0:
            return path, None
        headers = (
            repository_path(entry["directory"], name, build_dir)
            for name in parse_dependencies(result.stdout)
        )
        return path, [header for header in headers if header is not None and header != path]

    includers = {}
    with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
        for path, headers in pool.map(scan, sorted(units.items())):
            if headers is None:
                return None
            for header in headers:
                includers.setdefault(header, set()).add(path)
    return includers


def changed_files(base):
    result = subprocess.run(
        ["git", "-C", str(REPO_ROOT), "diff", "--name-only", "--no-renames", base, "--"],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise EnvironmentProblem(f"git diff against {base} failed: {result.stderr.strip()}")
    return sorted({line for line in result.stdout.splitlines() if line})


def source_files():
    """Tracked C sources and headers, or every one on disk outside a Git checkout."""
    try:
        output = subprocess.run(
            ["git", "-C", str(REPO_ROOT), "ls-files", "-z", "--", *SOURCE_PATHSPECS],
            check=True,
            capture_output=True,
        ).stdout
        return sorted(entry.decode() for entry in output.split(b"\0") if entry)
    except (OSError, subprocess.CalledProcessError):
        found = []
        for pathspec in SOURCE_PATHSPECS:
            top = pathspec.split("/")[0]
            for suffix in ("*.c", "*.h"):
                found += [
                    path.relative_to(REPO_ROOT).as_posix()
                    for path in (REPO_ROOT / top).rglob(suffix)
                ]
        return sorted(found)


def suppression_problems(files, enabled):
    """Report NOLINT comments that do not name enabled checks or give a reason.

    files yields (path, text) pairs. A name outside enabled is either not a
    check at all or one .clang-tidy disables; either way the comment silences
    nothing, which is what retired the clang-analyzer-valist.Uninitialized
    comments this gate replaced.
    """
    problems = []
    for path, text in files:
        for number, line in enumerate(text.splitlines(), 1):
            for match in SUPPRESSION_PATTERN.finditer(line):
                kind = "NOLINT" + (match.group(1) or "")
                where = f"{path}:{number}"
                form = SUPPRESSION_CHECKS.match(line, match.end())
                if form is None:
                    problems.append(f"{where}: {kind} must name the checks it silences")
                    continue
                checks = [name.strip() for name in form.group("checks").split(",")]
                for name in checks:
                    if not name or "*" in name:
                        problems.append(f"{where}: {kind} must name each check exactly: '{name}'")
                    elif not name.startswith("clang-diagnostic-") and name not in enabled:
                        problems.append(
                            f"{where}: {kind} names {name}, which .clang-tidy does not enable"
                        )
                if kind != "NOLINTEND" and not SUPPRESSION_REASON.match(form.group("rest")):
                    problems.append(
                        f"{where}: {kind}({form.group('checks')}) needs a reason after ' -- '"
                    )
    return problems


def parse_output(text, directory, build_dir):
    """Return ({(path, line, column, check): message}, [analysis error lines])."""
    sites = {}
    errors = []
    for line in text.splitlines():
        match = DIAGNOSTIC_PATTERN.match(line)
        if match is None:
            if line.startswith("Error while processing"):
                errors.append(line)
            continue
        checks = [
            name
            for name in (match.group("checks") or "").split(",")
            if name and name != "-warnings-as-errors"
        ]
        if match.group("severity") == "error" and (
            not checks or checks[0].startswith("clang-diagnostic-")
        ):
            errors.append(line)
            continue
        path = repository_path(directory, match.group("path"), build_dir)
        if path is None or not checks:
            continue
        site = (path, int(match.group("line")), int(match.group("column")), checks[0])
        sites.setdefault(site, match.group("message"))
    return sites, errors


def compare(counts, baseline, complete):
    """Return (failures, improvements) as ((path, check), now, allowed) tuples.

    complete(path) says whether this run saw every finding located in path.
    """
    failures = []
    improvements = []
    for key in sorted(set(counts) | set(baseline)):
        if not complete(key[0]):
            continue
        now = counts.get(key, 0)
        allowed = baseline.get(key, 0)
        if now > allowed:
            failures.append((key, now, allowed))
        elif now < allowed:
            improvements.append((key, now, allowed))
    return failures, improvements


def call_text(path, line, column):
    """The call at line:column with its arguments, whitespace collapsed.

    A site keeps its identity across a reformat and across edits elsewhere in
    the file, so the text runs from the callee to its matching parenthesis and
    every run of whitespace, including a line break, becomes one space.
    """
    try:
        source = (REPO_ROOT / path).read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return ""
    if not 1 <= line <= len(source):
        return ""
    text = "\n".join(source[line - 1 : line - 1 + SITE_TEXT_LINES])
    index = start = column - 1
    depth = 0
    quote = ""
    while index < len(text):
        character = text[index]
        if quote:
            if character == "\\":
                index += 2
                continue
            if character == quote:
                quote = ""
        elif character in "\"'":
            quote = character
        elif character == "(":
            depth += 1
        elif character == ")":
            depth -= 1
            if depth == 0:
                index += 1
                break
        index += 1
    collapsed = " ".join(text[start:index].split())
    return collapsed.encode("ascii", "replace").decode("ascii")


def read_sites():
    """{(path, call): count} from the recorded unsafe-call sites."""
    sites = {}
    if not SITES_PATH.exists():
        return sites
    for raw in SITES_PATH.read_text(encoding="ascii").splitlines():
        raw = raw.strip()
        if raw and not raw.startswith("#"):
            path, count, call = raw.split(None, 2)
            sites[(path, call)] = int(count)
    return sites


def write_sites(counts, version):
    with SITES_PATH.open("w", encoding="ascii", newline="\n") as handle:
        handle.write(
            f"# Every {SITE_TRACKED_CHECK} call clang-tidy {version} reports, as\n"
            "# 'file count call' (issue #89 ratchet). The per-file counts in\n"
            "# clang_tidy_baseline.txt cannot see one of these swapped for another, so the\n"
            "# call text is recorded too. A call that is not listed fails the check.\n"
            "# Record an edit to a listed call with --update, which never raises a count:\n"
            "#   scripts/ci/check_clang_tidy.py --build-dir build/analysis --update\n"
        )
        for (path, call), count in sorted(counts.items()):
            if count:
                handle.write(f"{path} {count} {call}\n")


def read_baseline():
    baseline = {}
    if not BASELINE_PATH.exists():
        return baseline
    for raw in BASELINE_PATH.read_text(encoding="ascii").splitlines():
        raw = raw.strip()
        if raw and not raw.startswith("#"):
            path, check, count = raw.split()
            baseline[(path, check)] = int(count)
    return baseline


def write_baseline(counts, version):
    with BASELINE_PATH.open("w", encoding="ascii", newline="\n") as handle:
        handle.write(
            "# clang-tidy findings per file and check (issue #89 ratchet), recorded with\n"
        )
        handle.write(
            f"# clang-tidy {version} and the tracked .clang-tidy. Distinct sites; a count\n"
        )
        handle.write("# may only shrink. After fixing findings, record the lower counts:\n")
        handle.write("#   scripts/ci/check_clang_tidy.py --build-dir build/analysis --update\n")
        handle.write("# When a file moves, rename its entries here in the same change.\n")
        for (path, check), count in sorted(counts.items()):
            if count:
                handle.write(f"{path} {check} {count}\n")


def pinned_version():
    for raw in REQUIREMENTS_PATH.read_text(encoding="ascii").splitlines():
        match = re.fullmatch(r"\s*clang-tidy==([0-9.]+)\s*(?:#.*)?", raw)
        if match:
            return match.group(1)
    raise EnvironmentProblem(f"{REQUIREMENTS_PATH} does not pin clang-tidy")


def tool_version(binary):
    try:
        output = subprocess.run(
            [binary, "--version"], capture_output=True, text=True, check=True
        ).stdout
    except (OSError, subprocess.CalledProcessError) as error:
        raise EnvironmentProblem(f"cannot run {binary}: {error}") from error
    match = re.search(r"LLVM version (\S+)", output)
    return match.group(1) if match else output.strip()


def enabled_checks(binary):
    """The checks the tracked configuration turns on, which is what a NOLINT may name.

    --checks='*' would list every check the binary has, so a suppression could
    name one this configuration disables and silence nothing.
    """
    output = subprocess.run(
        [binary, "--list-checks", f"--config-file={CONFIG_PATH}"],
        capture_output=True,
        text=True,
        check=True,
        cwd=REPO_ROOT,
    ).stdout
    return {line.strip() for line in output.splitlines() if line.startswith("    ")}


def analyze(binary, database_dir, entry):
    source = os.path.join(entry["directory"], entry["file"])
    return subprocess.run(
        [binary, "-p", database_dir, f"--config-file={CONFIG_PATH}", "--quiet", source],
        capture_output=True,
        text=True,
        cwd=REPO_ROOT,
    )


def run(args):
    build_dir = Path(os.path.realpath(args.build_dir))
    database_path = build_dir / "compile_commands.json"
    if not database_path.is_file():
        raise EnvironmentProblem(f"{database_path} does not exist; run cmake --preset analysis")
    pinned = pinned_version()
    version = tool_version(args.clang_tidy)
    if version != pinned:
        raise EnvironmentProblem(
            f"{args.clang_tidy} is version {version}, but the baseline is recorded with "
            f"{pinned}; install it with python3 -m pip install -r "
            f"{REQUIREMENTS_PATH.relative_to(REPO_ROOT)}"
        )
    database = json.loads(database_path.read_text())
    problem = database_compiler_problem(database)
    if problem:
        raise EnvironmentProblem(problem)

    coverage = coverage_problems(database, build_dir, (REPO_ROOT / "Makefile.am").read_text())
    checks = enabled_checks(args.clang_tidy)
    suppressions = suppression_problems(
        ((path, (REPO_ROOT / path).read_text(errors="replace")) for path in source_files()),
        checks,
    )
    units = select_units(database, build_dir)

    mode = "full"
    selected = set(units)
    includers = None
    if args.base:
        changed = changed_files(args.base)
        if any(path in FULL_RUN_PATHS or path.startswith(FULL_RUN_PREFIXES) for path in changed):
            print(f"the analysis configuration changed since {args.base}; analyzing every unit")
        else:
            mode = "changed"
            selected = {path for path in changed if path in units}
            headers = {path for path in changed if path.endswith(".h")}
            if headers:
                includers = include_map(units, build_dir, args.jobs)
                if includers is None:
                    print("the include scan failed; analyzing every unit")
                    mode = "full"
                    selected = set(units)
                else:
                    for header in headers:
                        selected |= includers.get(header, set())

    def complete(path):
        if mode == "full":
            return True
        if path in units:
            return path in selected
        users = includers.get(path) if includers else None
        return bool(users) and users <= selected

    print(
        f"clang-tidy {version}: {mode} analysis of {len(selected)} of {len(units)} "
        f"translation units with {args.jobs} jobs"
    )
    start = time.monotonic()
    outputs = {}
    sites = {}
    errors = []
    if selected:
        with tempfile.TemporaryDirectory(prefix="luminari-clang-tidy-") as database_dir:
            Path(database_dir, "compile_commands.json").write_text(
                json.dumps([units[path] for path in sorted(selected)])
            )
            with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
                results = pool.map(
                    lambda path: (path, analyze(args.clang_tidy, database_dir, units[path])),
                    sorted(selected),
                )
                for path, result in results:
                    text = result.stdout + result.stderr
                    outputs[path] = text
                    unit_sites, unit_errors = parse_output(
                        text, units[path]["directory"], build_dir
                    )
                    for site, message in unit_sites.items():
                        sites.setdefault(site, message)
                    if unit_errors or result.returncode != 0:
                        errors.append((path, result.returncode, unit_errors))
    seconds = time.monotonic() - start

    counts = Counter((path, check) for path, _, _, check in sites)
    site_counts = Counter(
        (path, call_text(path, line, column))
        for path, line, column, check in sites
        if check == SITE_TRACKED_CHECK
    )
    first_baseline = args.update and not BASELINE_PATH.exists()
    baseline = read_baseline()
    failures, improvements = compare(counts, baseline, complete)
    site_failures, site_improvements = compare(site_counts, read_sites(), complete)

    report_dir = Path(args.report_dir or build_dir / "clang-tidy-report")
    report_dir.mkdir(parents=True, exist_ok=True)
    with (report_dir / "clang-tidy.log").open("w", encoding="utf-8") as log:
        log.write(f"clang-tidy {version}, {mode} analysis, base {args.base or 'none'}\n")
        for path in sorted(outputs):
            log.write(f"==> {path} ({target_name(units[path]) or 'unknown target'})\n")
            log.write(outputs[path])
    report = {
        "clang_tidy_version": version,
        "config": CONFIG_PATH.name,
        "mode": mode,
        "base": args.base,
        "seconds": round(seconds, 1),
        "translation_units": sorted(selected),
        "diagnostics": [
            {"file": path, "line": line, "column": column, "check": check, "message": message}
            for (path, line, column, check), message in sorted(sites.items())
        ],
        "analysis_errors": [
            {"file": path, "status": status, "errors": lines} for path, status, lines in errors
        ],
        "coverage_problems": coverage,
        "suppression_problems": suppressions,
        "failures": [
            {"file": path, "check": check, "count": now, "baseline": allowed}
            for (path, check), now, allowed in failures
        ],
        "improvements": [
            {"file": path, "check": check, "count": now, "baseline": allowed}
            for (path, check), now, allowed in improvements
        ],
        "unsafe_call_failures": [
            {"file": path, "call": call, "count": now, "baseline": allowed}
            for (path, call), now, allowed in site_failures
        ],
    }
    (report_dir / "clang-tidy-report.json").write_text(json.dumps(report, indent=2) + "\n")

    print(
        f"{len(sites)} distinct findings in {seconds:.0f} s; complete log and JSON report in "
        f"{report_dir}"
    )
    status = 0
    for line in coverage:
        print(f"coverage: {line}", file=sys.stderr)
        status = 1
    for line in suppressions:
        print(f"suppression: {line}", file=sys.stderr)
        status = 1
    for path, returncode, lines in errors:
        print(f"analysis error in {path} (exit status {returncode}):", file=sys.stderr)
        for line in lines or outputs[path].splitlines():
            print(f"  {line}", file=sys.stderr)
        status = 1
    if failures and not first_baseline:
        print("clang-tidy findings above the baseline:", file=sys.stderr)
        for (path, check), now, allowed in failures:
            print(f"{path} {check}: {now} findings, baseline {allowed}", file=sys.stderr)
            for site_path, line, column, site_check in sorted(sites):
                if site_path == path and site_check == check:
                    message = sites[(site_path, line, column, site_check)]
                    print(f"  {path}:{line}:{column}: {message} [{check}]", file=sys.stderr)
        print(
            "fix the new findings, or suppress a false positive with "
            "/* NOLINTNEXTLINE(check) -- reason */; the full log names every site",
            file=sys.stderr,
        )
        status = 1
    for (path, check), now, allowed in improvements:
        print(f"improved: {path} {check}: {now} findings, baseline {allowed}")
    if improvements:
        print(
            "record the lower counts with: scripts/ci/check_clang_tidy.py --build-dir BUILD --update"
        )
    # A site list failure does not block --update: editing a recorded call
    # changes its text without adding a call, and the counts above are what
    # --update refuses to raise.
    blocked = status
    if site_failures and not first_baseline:
        print(f"{SITE_TRACKED_CHECK} calls that are not recorded:", file=sys.stderr)
        for (path, call), now, allowed in site_failures:
            print(f"{path}: {call} ({now} of them, recorded {allowed})", file=sys.stderr)
        print(
            f"bind the call, or record an edit to a listed one with --update; "
            f"{SITES_PATH.relative_to(REPO_ROOT)} holds every recorded call",
            file=sys.stderr,
        )
        status = 1
    for (path, call), now, allowed in site_improvements:
        print(f"improved: {path}: {call} ({now} of them, recorded {allowed})")

    if args.update:
        if blocked:
            print(
                "refusing to update the baseline until the problems above are fixed",
                file=sys.stderr,
            )
            return 1
        write_baseline(counts, version)
        write_sites(site_counts, version)
        print(
            f"wrote {BASELINE_PATH.relative_to(REPO_ROOT)} and {SITES_PATH.relative_to(REPO_ROOT)}"
        )
        return 0
    if status == 0:
        print("clang-tidy baseline respected")
    return status


def self_test():
    build_dir = Path(os.path.realpath(REPO_ROOT / "build" / "self-test"))
    root = REPO_ROOT.as_posix()
    output = "\n".join(
        [
            f"{root}/src/core/a.c:10:5: warning: assignment in if [bugprone-assignment-in-if-condition]",
            f"{root}/src/core/a.c:10:5: warning: assignment in if [bugprone-assignment-in-if-condition]",
            f"{root}/src/core/a.h:3:1: warning: no default [bugprone-switch-missing-default-case,-warnings-as-errors]",
            f"{root}/src/core/a.c:12:1: note: Taking true branch",
            "/usr/include/stdio.h:1:1: warning: outside the tree [bugprone-unsafe-functions]",
            f"{build_dir}/AllTests.c:1:1: warning: generated [bugprone-branch-clone]",
            f"{root}/src/core/b.c:2:3: error: unknown type name 'foo' [clang-diagnostic-error]",
            "Error while processing /tmp/b.c.",
        ]
    )
    sites, errors = parse_output(output, root, build_dir)
    assert sorted(sites) == [
        ("src/core/a.c", 10, 5, "bugprone-assignment-in-if-condition"),
        ("src/core/a.h", 3, 1, "bugprone-switch-missing-default-case"),
    ], sites
    assert len(errors) == 2, errors

    assert parse_dependencies("a.o: src/a.c src/a.h \\\n  src/core/structs.h\n") == [
        "src/a.c",
        "src/a.h",
        "src/core/structs.h",
    ]

    counts = Counter({("src/a.c", "c1"): 2, ("src/a.h", "c2"): 1, ("src/b.c", "c1"): 1})
    baseline = {("src/a.c", "c1"): 1, ("src/b.c", "c1"): 3, ("src/c.c", "c3"): 1}
    failures, improvements = compare(counts, baseline, lambda path: path != "src/a.h")
    assert failures == [(("src/a.c", "c1"), 2, 1)], failures
    assert improvements == [(("src/b.c", "c1"), 1, 3), (("src/c.c", "c3"), 0, 1)], improvements
    failures, _ = compare(counts, baseline, lambda path: True)
    assert (("src/a.h", "c2"), 1, 0) in failures, failures

    # clang-analyzer-security.VAList exists in the binary but .clang-tidy
    # disables it, so it is not in the enabled set and may not be named.
    enabled = {"bugprone-branch-clone"}
    text = "\n".join(
        [
            "/* NOLINTNEXTLINE(bugprone-branch-clone) -- the arms differ by a constant. */",
            "x = 1; // NOLINT",
            "/* NOLINTNEXTLINE(bugprone-branch-clone) */",
            "/* NOLINTBEGIN(*) -- everything */",
            "/* NOLINTEND(bugprone-branch-clone) */",
            "/* NOLINT(clang-analyzer-valist.Uninitialized) -- renamed check */",
            "/* NOLINT(clang-analyzer-security.VAList) -- disabled in .clang-tidy */",
            "y = 2; /* NOLINT(clang-diagnostic-unused-variable) -- compiler warning */",
        ]
    )
    problems = suppression_problems([("src/a.c", text)], enabled)
    assert problems == [
        "src/a.c:2: NOLINT must name the checks it silences",
        "src/a.c:3: NOLINTNEXTLINE(bugprone-branch-clone) needs a reason after ' -- '",
        "src/a.c:4: NOLINTBEGIN must name each check exactly: '*'",
        "src/a.c:6: NOLINT names clang-analyzer-valist.Uninitialized, "
        "which .clang-tidy does not enable",
        "src/a.c:7: NOLINT names clang-analyzer-security.VAList, which .clang-tidy does not enable",
    ], problems

    probe = REPO_ROOT / "src" / "olc" / "improved-edit.c"
    text = probe.read_text(encoding="utf-8", errors="replace").splitlines()
    line = next(
        number for number, source in enumerate(text, 1) if "snprintf(buf + length" in source
    )
    column = text[line - 1].index("snprintf") + 1
    # The call wraps onto the next line; the recorded text is one line either way.
    assert call_text("src/olc/improved-edit.c", line, column) == (
        'snprintf(buf + length, sizeof(buf) - length, "\\r\\n%u line%sshown.\\r\\n", total_len, '
        '(total_len != 1) ? "s " : " ")'
    ), call_text("src/olc/improved-edit.c", line, column)
    assert call_text("src/olc/improved-edit.c", len(text) + 10, 1) == ""
    assert call_text("src/does/not/exist.c", 1, 1) == ""

    sites = Counter({("src/a.c", 'sprintf(b, "%s", n)'): 2, ("src/a.c", "strcpy(a, b)"): 1})
    recorded = {("src/a.c", 'sprintf(b, "%s", n)'): 2, ("src/a.c", "strcat(a, b)"): 1}
    # One recorded call swapped for another leaves the file's count unchanged.
    assert sum(sites.values()) == sum(recorded.values())
    failures, improvements = compare(sites, recorded, lambda path: True)
    assert failures == [(("src/a.c", "strcpy(a, b)"), 1, 0)], failures
    assert improvements == [(("src/a.c", "strcat(a, b)"), 0, 1)], improvements

    database = [
        {
            "directory": build_dir.as_posix(),
            "file": f"{root}/src/core/a.c",
            "output": "CMakeFiles/cutest.dir/src/core/a.c.o",
            "command": "/usr/bin/clang -DLUMINARI_CUTEST -c src/core/a.c",
        },
        {
            "directory": build_dir.as_posix(),
            "file": f"{root}/src/core/a.c",
            "output": "CMakeFiles/luminari.dir/src/core/a.c.o",
            "command": "/usr/bin/clang -c src/core/a.c",
        },
        {
            "directory": build_dir.as_posix(),
            "file": f"{build_dir}/AllTests.c",
            "output": "CMakeFiles/cutest.dir/AllTests.c.o",
            "command": "/usr/bin/clang -c AllTests.c",
        },
        {
            "directory": build_dir.as_posix(),
            "file": f"{root}/unittests/CuTest/test_a.c",
            "output": "CMakeFiles/cutest.dir/unittests/CuTest/test_a.c.o",
            "command": "/usr/bin/clang -c unittests/CuTest/test_a.c",
        },
    ]
    units = select_units(database, build_dir)
    assert sorted(units) == ["src/core/a.c", "unittests/CuTest/test_a.c"], units
    assert target_name(units["src/core/a.c"]) == "luminari"
    makefile = (
        "luminari_SOURCES = src/core/a.c src/core/a.h \\\n\tsrc/core/missing.c\n"
        "cutest_test_files = unittests/CuTest/test_a.c\n"
    )
    assert coverage_problems(database, build_dir, makefile) == [
        "the compilation database has no luminari command for src/core/missing.c (luminari_SOURCES)"
    ]
    assert database_compiler_problem(database) is None
    assert database_compiler_problem([{"command": "/usr/bin/gcc -c a.c"}]) is not None
    assert re.fullmatch(r"[0-9.]+", pinned_version())
    print("check_clang_tidy self-test passed")


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument(
        "--build-dir",
        type=Path,
        help="CMake build directory holding compile_commands.json (cmake --preset analysis)",
    )
    parser.add_argument("--base", metavar="REF", help="analyze only what git diff REF affects")
    parser.add_argument(
        "--clang-tidy", default="clang-tidy", help="clang-tidy executable (default: from PATH)"
    )
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    parser.add_argument(
        "--report-dir",
        type=Path,
        help="directory for the complete log and JSON report (default: BUILD_DIR/clang-tidy-report)",
    )
    parser.add_argument(
        "--update", action="store_true", help="record lower counts from a full run; refuses growth"
    )
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    if not args.build_dir:
        parser.error("--build-dir is required")
    if args.update and args.base:
        parser.error("--update analyzes the whole tree; drop --base")
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    try:
        return run(args)
    except EnvironmentProblem as problem:
        print(problem, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
