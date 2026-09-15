#!/usr/bin/env python3
"""Ratchet header self-containment.

Every header under src/ must compile on its own: a translation unit holding
nothing but ``#include "dir/header.h"`` has to pass ``-fsyntax-only -Werror``
with only the build root (for the generated conf.h) and src/ on the include
path. The compiler's default diagnostics count as errors, so a type or macro
the header uses without declaring it (an implicit int, a struct first seen in
a parameter list) is reported as the missing include it is.

Headers that still depend on what their includer included first are listed in
scripts/ci/header_self_containment_baseline.txt. The list may only shrink: an
unlisted header that fails fails the check, and a listed header that now
compiles on its own is reported so ``--update`` can drop it. The compilers
disagree about a few default diagnostics, so pass every supported compiler to
``--update``; it removes only the headers that pass with all of them. Without a
baseline file, ``--update`` records every failing header as the first one.

Usage:
  check_header_self_containment.py --build-root DIR [--cc CC ...]            # compare
  check_header_self_containment.py --build-root DIR --cc gcc --cc clang --update
"""

import argparse
import concurrent.futures
import os
import shlex
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
BASELINE_PATH = REPO_ROOT / "scripts" / "ci" / "header_self_containment_baseline.txt"
STD_FLAGS = ("-std=gnu23", "-std=gnu2x")


def list_headers():
    """Tracked headers under src/, or every header there outside a Git checkout."""
    try:
        output = subprocess.run(
            ["git", "-C", str(REPO_ROOT), "ls-files", "-z", "--", "src/*.h"],
            check=True,
            capture_output=True,
        ).stdout
        return sorted(entry.decode() for entry in output.split(b"\0") if entry)
    except (OSError, subprocess.CalledProcessError):
        # An exported archive has no Git metadata; its local config headers
        # are copies of the tracked examples and are checked like them.
        return sorted(str(path.relative_to(REPO_ROOT)) for path in (REPO_ROOT / "src").rglob("*.h"))


def compiler_flags(cc):
    """Return the language flags for cc: GNU C23 and any host-local driver suppression."""
    for std in STD_FLAGS:
        probe = subprocess.run(
            [*shlex.split(cc), std, "-fsyntax-only", "-Werror", "-x", "c", "-"],
            input="int probe;\n",
            capture_output=True,
            text=True,
        )
        if probe.returncode == 0:
            return [std]
        # Same local suppression as scripts/deployment/production_profile.sh:
        # a Clang host with several GCC installations emits this driver note
        # on every invocation. It concerns the C++ library search path only.
        if "-Wgcc-install-dir-libstdcxx" in probe.stderr:
            quiet = [std, "-Wno-gcc-install-dir-libstdcxx"]
            retry = subprocess.run(
                [*shlex.split(cc), *quiet, "-fsyntax-only", "-Werror", "-x", "c", "-"],
                input="int probe;\n",
                capture_output=True,
                text=True,
            )
            if retry.returncode == 0:
                return quiet
    raise SystemExit(
        f"{cc} compiles an empty translation unit with neither {' nor '.join(STD_FLAGS)}"
    )


def check_header(command, header):
    """Return (header, compiles, diagnostics) for one header on its own."""
    include = header.removeprefix("src/")
    result = subprocess.run(
        command, input=f'#include "{include}"\n', capture_output=True, text=True
    )
    return header, result.returncode == 0, result.stderr


def read_baseline():
    if not BASELINE_PATH.exists():
        return []
    entries = []
    for raw in BASELINE_PATH.read_text(encoding="ascii").splitlines():
        raw = raw.strip()
        if raw and not raw.startswith("#"):
            entries.append(raw)
    return entries


def write_baseline(entries):
    with BASELINE_PATH.open("w", encoding="ascii", newline="\n") as handle:
        handle.write("# Headers under src/ that do not compile on their own yet (issue #89).\n")
        handle.write("# The list may only shrink: every other header must compile alone. After\n")
        handle.write("# fixing headers, drop the ones that pass with every supported compiler:\n")
        handle.write(
            "#   scripts/ci/check_header_self_containment.py --build-root BUILD"
            " --cc gcc --cc clang --update\n"
        )
        for entry in sorted(entries):
            handle.write(entry + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument(
        "--cc",
        action="append",
        help="compiler command; repeat for several (default: $CC, else cc)",
    )
    parser.add_argument(
        "--build-root",
        type=Path,
        required=True,
        help="configured build directory that holds the generated conf.h",
    )
    parser.add_argument(
        "--update",
        action="store_true",
        help="drop listed headers that now pass with every given compiler; refuses additions",
    )
    args = parser.parse_args()
    compilers = args.cc or [os.environ.get("CC") or "cc"]
    build_root = args.build_root.resolve()
    if not (build_root / "conf.h").is_file():
        parser.error(f"{build_root / 'conf.h'} does not exist; configure the build first")

    headers = list_headers()
    if not headers:
        print("no headers found under src/", file=sys.stderr)
        return 1
    baseline = read_baseline()
    listed = set(baseline)
    failed_unlisted = []
    passing_everywhere = set(listed)

    for cc in compilers:
        command = [
            *shlex.split(cc),
            *compiler_flags(cc),
            "-fsyntax-only",
            "-Werror",
            "-I",
            str(build_root),
            "-I",
            str(REPO_ROOT / "src"),
            "-x",
            "c",
            "-",
        ]
        with concurrent.futures.ThreadPoolExecutor(os.cpu_count() or 1) as pool:
            results = list(pool.map(lambda header: check_header(command, header), headers))
        passed = {header for header, compiles, _ in results if compiles}
        passing_everywhere &= passed
        for header, compiles, diagnostics in results:
            if not compiles and header not in listed:
                failed_unlisted.append((cc, header, diagnostics))
        print(
            f"{cc}: {len(passed)} of {len(headers)} headers compile on their own; "
            f"{len(listed)} listed exceptions"
        )

    stale = sorted(listed - set(headers))
    now_passing = sorted(passing_everywhere - set(stale))

    if failed_unlisted:
        print("headers that do not compile on their own:", file=sys.stderr)
        for cc, header, diagnostics in failed_unlisted:
            print(f"--- {header} ({cc})", file=sys.stderr)
            print(diagnostics.rstrip(), file=sys.stderr)
        print(
            "include what each header uses (or forward-declare the struct) so it compiles alone",
            file=sys.stderr,
        )
    if args.update:
        if not BASELINE_PATH.exists():
            write_baseline({header for _, header, _ in failed_unlisted})
            print(f"wrote the first {BASELINE_PATH.relative_to(REPO_ROOT)}")
            return 0
        if failed_unlisted:
            print("refusing to update: the baseline never grows", file=sys.stderr)
            return 1
        remaining = [entry for entry in baseline if entry not in now_passing and entry not in stale]
        write_baseline(remaining)
        print(f"wrote {BASELINE_PATH.relative_to(REPO_ROOT)} ({len(remaining)} headers)")
        return 0
    for header in stale:
        print(f"listed header no longer exists: {header}")
    for header in now_passing:
        print(f"now compiles on its own: {header}")
    if stale or now_passing:
        print("drop them with --update, passing every supported compiler")
    return 1 if failed_unlisted else 0


if __name__ == "__main__":
    sys.exit(main())
