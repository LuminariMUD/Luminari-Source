#!/usr/bin/env python3
"""Measure how well the focused tests catch deliberate faults in critical code.

Each module below is a parser or authentication source with a standalone test
harness. For every relational operator (< <= > >= == !=), logical operator
(&& ||), and boolean return (true/false, TRUE/FALSE) in the module, or in the
listed functions of a large module, the check compiles a copy of the module with
that one token replaced, links it into the harness, and runs the harness. A
mutant is killed when the harness fails or times out and survives when it
passes. A mutant whose object file is identical to the unmodified module's (a
token in a disabled preprocessor branch, for example) is equivalent, and one
that does not compile is invalid; neither counts.

The mutation score, killed / (killed + survived), is held to the floor recorded
for the module in scripts/ci/coverage_policy.json. Floors only rise
(scripts/ci/check_baseline_ratchet.py). Surviving mutants are listed: each is a
fault the tests accept.

Needs a configured tree (conf.h), a C compiler (CC, default cc), json-c, and
libxcrypt.

Usage:
  mutation_test.py [--module SOURCE]... [--jobs N] [--report FILE]
  mutation_test.py --self-test
"""

import argparse
import concurrent.futures
import json
import math
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass, field
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[1]
POLICY_PATH = SCRIPT_DIR / "coverage_policy.json"
CUTEST = "unittests/CuTest"
# A mutant that runs this many times longer than the unmodified harness (and at
# least the minimum) is stuck, which the tests have detected.
TIMEOUT_FACTOR = 10
MINIMUM_TIMEOUT_SECONDS = 5.0

OPERATORS = {"<": "<=", "<=": "<", ">": ">=", ">=": ">", "==": "!=", "!=": "==", "&&": "||"}
OPERATORS["||"] = "&&"
# Longer tokens first, so << >> -> and compound assignments are never split.
OPERATOR_TOKEN = re.compile(r"<<=|>>=|<<|>>|->|<=|>=|==|!=|&&|\|\||<|>")
BOOLEAN_RETURN = re.compile(r"\breturn\s+(true|false|TRUE|FALSE)\s*;")
FLIPPED = {"true": "false", "false": "true", "TRUE": "FALSE", "FALSE": "TRUE"}

# The password harness links password.c without the game's logging layer.
LOG_STUB = """#include <stdio.h>
FILE *logfile;
void basic_mud_log(const char *format, ...);
void basic_mud_log(const char *format, ...) { (void)format; }
"""


@dataclass
class Module:
    source: str
    # Only these functions are mutated; None mutates the whole file.
    functions: tuple = None
    defines: tuple = ()
    # (source, defines) linked with every mutant; "runner:<test file>" is the
    # generated CuTest runner for that file and "log-stub" is LOG_STUB.
    harness: tuple = ()
    libraries: tuple = ()


MODULES = (
    Module(
        source="src/core/binary_formats.c",
        defines=("-DBINARY_FORMATS_FOCUSED_HARNESS",),
        harness=(
            (f"{CUTEST}/CuTest.c", ()),
            (f"{CUTEST}/test_binary_formats.c", ("-DBINARY_FORMATS_FOCUSED_HARNESS",)),
            (f"runner:{CUTEST}/test_binary_formats.c", ()),
        ),
        libraries=("-lm",),
    ),
    Module(
        source="src/net/protocol.c",
        functions=(
            "ProtocolInput",
            "PerformHandshake",
            "PerformSubnegotiation",
            "ParseMSDP",
            "ExecuteMSDPPair",
            "ParseGMCP",
            "ValidateMSDPValue",
            "gmcp_msdp_scalar_is_valid",
            "gmcp_msdp_value_is_valid",
            "gmcp_json_contains_nul",
        ),
        defines=("-DLUMINARI_PROTOCOL_TEST=1",),
        harness=(
            (f"{CUTEST}/CuTest.c", ()),
            (f"{CUTEST}/test_protocol_parser.c", ("-DLUMINARI_PROTOCOL_TEST=1",)),
            ("src/net/msdp_json.c", ("-DLUMINARI_PROTOCOL_TEST=1",)),
            ("src/net/onboarding.c", ("-DWEB_ONBOARDING_FOCUSED_PROTOCOL_HARNESS=1",)),
        ),
        libraries=("-lm", "-ljson-c"),
    ),
    Module(
        source="src/player/password.c",
        harness=(
            (f"{CUTEST}/CuTest.c", ()),
            (f"{CUTEST}/test_password.c", ()),
            (f"runner:{CUTEST}/test_password.c", ()),
            ("log-stub", ()),
        ),
        libraries=("-lcrypt", "-lm"),
    ),
)


@dataclass
class Mutant:
    line: int
    column: int
    offset: int
    original: str
    replacement: str

    def describe(self, source):
        return f"{source}:{self.line}:{self.column}: {self.original!r} -> {self.replacement!r}"


@dataclass
class Result:
    module: str
    killed: int = 0
    survived: list = field(default_factory=list)
    equivalent: int = 0
    invalid: int = 0

    def score(self):
        tested = self.killed + len(self.survived)
        return 100.0 * self.killed / tested if tested else 0.0


def mask_non_code(text):
    """text with comments, string and character literals, and preprocessor
    lines replaced by spaces (newlines kept), so offsets still line up."""
    masked = list(text)
    index = 0
    length = len(text)
    line_start = True
    while index < length:
        char = text[index]
        if line_start and char in " \t":
            index += 1
            continue
        if line_start and char == "#":
            end = index
            while end < length and not (text[end] == "\n" and text[end - 1] != "\\"):
                end += 1
            for position in range(index, end):
                if text[position] != "\n":
                    masked[position] = " "
            index = end
            continue
        line_start = char == "\n"
        if text.startswith("//", index):
            end = text.find("\n", index)
            end = length if end < 0 else end
        elif text.startswith("/*", index):
            end = text.find("*/", index + 2)
            end = length if end < 0 else end + 2
        elif char in "\"'":
            end = index + 1
            while end < length and text[end] != char:
                end += 2 if text[end] == "\\" else 1
            end = min(end + 1, length)
        else:
            index += 1
            continue
        for position in range(index, end):
            if text[position] != "\n":
                masked[position] = " "
        index = end
    return "".join(masked)


def function_spans(masked, names):
    """{name: (start, end)} of each named function body in masked source."""
    spans = {}
    for name in names:
        pattern = re.compile(
            r"^[A-Za-z_][\w \t\*]*\b" + re.escape(name) + r"\s*\([^;{)]*\)\s*\{", re.MULTILINE
        )
        match = pattern.search(masked)
        if not match:
            raise SystemExit(f"function {name} was not found")
        depth = 0
        for index in range(match.end() - 1, len(masked)):
            if masked[index] == "{":
                depth += 1
            elif masked[index] == "}":
                depth -= 1
                if depth == 0:
                    spans[name] = (match.end() - 1, index + 1)
                    break
    return spans


def find_mutants(text, functions=None):
    masked = mask_non_code(text)
    spans = function_spans(masked, functions).values() if functions else [(0, len(text))]
    mutants = []
    for start, end in sorted(spans):
        region = masked[start:end]
        for match in OPERATOR_TOKEN.finditer(region):
            token = match.group(0)
            if token in OPERATORS:
                mutants.append((start + match.start(), token, OPERATORS[token]))
        for match in BOOLEAN_RETURN.finditer(region):
            mutants.append((start + match.start(1), match.group(1), FLIPPED[match.group(1)]))
    result = []
    for offset, original, replacement in sorted(mutants):
        line = text.count("\n", 0, offset) + 1
        column = offset - (text.rfind("\n", 0, offset) + 1) + 1
        result.append(Mutant(line, column, offset, original, replacement))
    return result


def apply_mutant(text, mutant):
    end = mutant.offset + len(mutant.original)
    assert text[mutant.offset : end] == mutant.original
    return text[: mutant.offset] + mutant.replacement + text[end:]


def compiler():
    return shlex.split(os.environ.get("CC", "cc"))


def include_flags(source):
    return [
        "-std=gnu2x",
        "-g0",
        "-O0",
        "-w",
        # As in unittests/CuTest/Makefile: the linker drops what the harness never
        # calls, such as the onboarding code that needs OpenSSL.
        "-ffunction-sections",
        "-fdata-sections",
        f"-I{REPO_ROOT}",
        f"-I{REPO_ROOT / 'src'}",
        f"-I{REPO_ROOT / CUTEST}",
        "-iquote",
        str((REPO_ROOT / source).parent),
    ]


def run(command, cwd, timeout=None):
    return subprocess.run(
        command, cwd=cwd, capture_output=True, text=True, timeout=timeout, check=False
    )


def build_harness(module, directory):
    """Compile the harness objects once; return their paths."""
    objects = []
    for index, (source, defines) in enumerate(module.harness):
        if source.startswith("runner:"):
            test_file = REPO_ROOT / source.removeprefix("runner:")
            generated = directory / "AllTests.c"
            generated.write_bytes(
                subprocess.check_output(
                    ["bash", str(REPO_ROOT / CUTEST / "make-tests.sh"), str(test_file)]
                )
            )
            path = generated
        elif source == "log-stub":
            path = directory / "log_stub.c"
            path.write_text(LOG_STUB, encoding="ascii")
        else:
            path = REPO_ROOT / source
        target = directory / f"harness{index}.o"
        result = run(
            [
                *compiler(),
                *include_flags(module.source),
                *defines,
                "-c",
                str(path),
                "-o",
                str(target),
            ],
            directory,
        )
        if result.returncode != 0:
            raise SystemExit(f"cannot build the {module.source} harness:\n{result.stderr}")
        objects.append(str(target))
    return objects


def compile_module(module, text, directory):
    """Compile text as the module inside directory; return the object path or None."""
    name = Path(module.source).name
    (directory / name).write_text(text, encoding="utf-8")
    result = run(
        [*compiler(), *include_flags(module.source), *module.defines, "-c", name, "-o", "module.o"],
        directory,
    )
    return directory / "module.o" if result.returncode == 0 else None


def link_and_run(module, obj, harness, directory, timeout):
    """(passed, output): passed is True or False, or None when obj cannot link."""
    executable = directory / "harness"
    result = run(
        [
            *compiler(),
            str(obj),
            *harness,
            *module.libraries,
            "-Wl,--gc-sections",
            "-o",
            str(executable),
        ],
        directory,
    )
    if result.returncode != 0:
        return None, result.stderr
    try:
        outcome = run([str(executable)], directory, timeout=timeout)
    except subprocess.TimeoutExpired:
        return False, "timed out"
    return outcome.returncode == 0, outcome.stdout + outcome.stderr


def test_module(module, jobs):
    text = (REPO_ROOT / module.source).read_text(encoding="utf-8")
    result = Result(module.source)
    with tempfile.TemporaryDirectory(prefix="luminari-mutation-") as scratch:
        scratch = Path(scratch)
        base = scratch / "original"
        base.mkdir()
        harness = build_harness(module, base)
        original = compile_module(module, text, base)
        if original is None:
            raise SystemExit(f"the unmodified {module.source} does not compile")
        started = time.monotonic()
        passed, output = link_and_run(module, original, harness, base, None)
        if not passed:
            raise SystemExit(f"the unmodified {module.source} does not pass its harness:\n{output}")
        timeout = max(MINIMUM_TIMEOUT_SECONDS, TIMEOUT_FACTOR * (time.monotonic() - started))
        original_bytes = original.read_bytes()
        mutants = find_mutants(text, module.functions)

        def attempt(numbered):
            number, mutant = numbered
            directory = scratch / f"mutant{number}"
            directory.mkdir()
            try:
                obj = compile_module(module, apply_mutant(text, mutant), directory)
                if obj is None:
                    return mutant, "invalid"
                if obj.read_bytes() == original_bytes:
                    return mutant, "equivalent"
                passed, _output = link_and_run(module, obj, harness, directory, timeout)
                if passed is None:
                    return mutant, "invalid"
                return mutant, "survived" if passed else "killed"
            finally:
                shutil.rmtree(directory, ignore_errors=True)

        with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
            for mutant, outcome in pool.map(attempt, enumerate(mutants)):
                if outcome == "killed":
                    result.killed += 1
                elif outcome == "survived":
                    result.survived.append(mutant)
                elif outcome == "equivalent":
                    result.equivalent += 1
                else:
                    result.invalid += 1
    return result


def floor2(value):
    return math.floor(value * 100.0 + 1e-9) / 100.0


def self_test():
    source = """#include <stdio.h>
#define LIMIT(a) ((a) < 3)
#define BOTH(a, b) \\
  ((a) < (b) && \\
   (b) < 3)
/* a < b in a comment */
static int helper(int a, int b)
{
  const char *text = "a < b && c";
  char quote = '<';
  if (a < b && b >= 2 || a == 0)
    return a != b;
  return p->x << 1 > 2;
}

bool other(int a)
{
  if (a <= 1)
    return true;
  return FALSE;
}
"""
    masked = mask_non_code(source)
    assert len(masked) == len(source) and masked.count("\n") == source.count("\n")
    assert (
        "#include" not in masked
        and "LIMIT" not in masked
        and "BOTH" not in masked
        and "comment" not in masked
    )
    assert '"' not in masked and "'" not in masked

    everything = [(m.line, m.original, m.replacement) for m in find_mutants(source)]
    assert everything == [
        (11, "<", "<="),
        (11, "&&", "||"),
        (11, ">=", ">"),
        (11, "||", "&&"),
        (11, "==", "!="),
        (12, "!=", "=="),
        (13, ">", ">="),
        (18, "<=", "<"),
        (19, "true", "false"),
        (20, "FALSE", "TRUE"),
    ], everything
    only_other = [(m.line, m.original) for m in find_mutants(source, ("other",))]
    assert only_other == [(18, "<="), (19, "true"), (20, "FALSE")], only_other

    mutant = find_mutants(source, ("other",))[0]
    assert mutant.column == 9, mutant
    assert "if (a < 1)" in apply_mutant(source, mutant)
    assert mutant.describe("x.c") == "x.c:18:9: '<=' -> '<'"

    policy = json.loads(POLICY_PATH.read_text(encoding="utf-8"))
    assert sorted(policy["mutation"]) == sorted(module.source for module in MODULES)
    for module in MODULES:
        text = (REPO_ROOT / module.source).read_text(encoding="utf-8")
        assert find_mutants(text, module.functions), module.source
        for source_file, _defines in module.harness:
            if ":" not in source_file and source_file != "log-stub":
                assert (REPO_ROOT / source_file).is_file(), source_file
    print("mutation_test self-test passed")


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--module", action="append", help="test only this source")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    parser.add_argument("--report", type=Path, help="also write the result to this file")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0

    floors = json.loads(POLICY_PATH.read_text(encoding="utf-8"))["mutation"]
    selected = [module for module in MODULES if not args.module or module.source in args.module]
    if args.module and len(selected) != len(set(args.module)):
        parser.error("--module must name a source listed in MODULES")
    lines = []
    failures = []
    for module in selected:
        result = test_module(module, max(args.jobs, 1))
        floor = floors[module.source]["score"]
        score = result.score()
        lines.append(
            f"{module.source}: score {score:.2f}% (floor {floor:.2f}); killed {result.killed},"
            f" survived {len(result.survived)}, equivalent {result.equivalent},"
            f" invalid {result.invalid}"
        )
        lines.extend(f"  survived: {mutant.describe(module.source)}" for mutant in result.survived)
        if score + 1e-9 < floor:
            failures.append(
                f"{module.source} mutation score {score:.2f}% is below its floor {floor:.2f}"
            )
        elif floor2(score) > floor:
            lines.append(
                f"improved: {module.source} floor can rise to {floor2(score):.2f};"
                f" record it in {POLICY_PATH.relative_to(REPO_ROOT)}"
            )
    lines.extend(f"FAILED: {line}" for line in failures)
    output = "\n".join(lines) + "\n"
    sys.stdout.write(output)
    if args.report:
        args.report.write_text(output, encoding="utf-8")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
