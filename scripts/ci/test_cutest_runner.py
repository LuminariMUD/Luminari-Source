#!/usr/bin/env python3
"""Exercise the generated CuTest runner: filtering, failed-test timing, and seeding."""

import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
CUTEST = ROOT / "unittests/CuTest"

# A random-number reseed that reads the clock makes a run unrepeatable.
CLOCK_RESEED = re.compile(r"\b(?:circle_srandom|srandom|srand)\s*\([^;]*\btime\s*\(")


def build_runner(root, source, *flags):
    """Generate a runner for source in root and compile it; return the executable."""
    fixture = root / "tests.c"
    fixture.write_text(source, encoding="ascii")
    generated = subprocess.check_output(["bash", str(CUTEST / "make-tests.sh"), str(fixture)])
    (root / "AllTests.c").write_bytes(generated)
    prototypes = subprocess.check_output(
        ["bash", str(CUTEST / "make-tests.sh"), "--prototypes", str(fixture)]
    )
    (root / "test_prototypes.h").write_bytes(prototypes)
    executable = root / "runner"
    subprocess.run(
        [
            *shlex.split(os.environ.get("CC", "cc")),
            "-std=gnu2x",
            "-Wall",
            "-Wextra",
            "-Werror",
            *flags,
            "-I",
            str(root),
            "-I",
            str(CUTEST),
            str(fixture),
            str(root / "AllTests.c"),
            str(CUTEST / "CuTest.c"),
            "-lm",
            "-o",
            str(executable),
        ],
        check=True,
    )
    return executable


def run(executable, **variables):
    env = dict(os.environ)
    env.pop("CUTEST_FILTER", None)
    env.pop("LUMINARI_TEST_SEED", None)
    env.update(variables)
    return subprocess.run(
        [str(executable)],
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )


class RunnerTest(unittest.TestCase):
    def test_generated_runner(self):
        with tempfile.TemporaryDirectory(prefix="cutest-runner-") as directory:
            executable = build_runner(
                Path(directory),
                """#include "CuTest.h"
#include <stdio.h>
#include <time.h>
FILE *logfile;
void Test_first(CuTest *tc) { CuAssertTrue(tc, 1); }
void Test_second(CuTest *tc) { CuAssertTrue(tc, 1); }
void Test_slow_failure(CuTest *tc)
{
  struct timespec delay = {1, 100000000};
  nanosleep(&delay, NULL);
  CuFail(tc, "intentional failure");
}
""",
            )

            full = run(executable)
            self.assertNotEqual(full.returncode, 0)
            self.assertIn("Runs: 3 Passes: 2 Fails: 1", full.stdout)
            self.assertNotIn("tests selected", full.stdout)
            self.assertIn("Slow test: Test_slow_failure (", full.stdout)
            self.assertNotIn("Slow test: Test_first", full.stdout)
            # Only the production-linked suite has random sources to seed.
            self.assertNotIn("LUMINARI_TEST_SEED", full.stdout)
            selected = run(executable, CUTEST_FILTER="first")
            self.assertEqual(selected.returncode, 0, selected.stdout)
            self.assertIn("OK (1 test)", selected.stdout)
            self.assertIn("CUTEST_FILTER=first: 1 of 3 tests selected", selected.stdout)
            self.assertNotIn("slow_failure", selected.stdout)
            missing = run(executable, CUTEST_FILTER="does_not_exist")
            self.assertNotEqual(missing.returncode, 0)
            self.assertIn("No tests matched CUTEST_FILTER=does_not_exist", missing.stdout)
            empty = run(executable, CUTEST_FILTER="")
            self.assertIn("Runs: 3 Passes: 2 Fails: 1", empty.stdout)
            self.assertNotIn("tests selected", empty.stdout)
            self.assertNotEqual(empty.returncode, 0)

    def test_production_runner_seeds_every_test(self):
        with tempfile.TemporaryDirectory(prefix="cutest-seed-") as directory:
            executable = build_runner(
                Path(directory),
                """#include "CuTest.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
FILE *logfile;
static unsigned long seeded;
void circle_srandom(unsigned long initial_seed);
void circle_srandom(unsigned long initial_seed) { seeded = initial_seed; }
static void report(CuTest *tc)
{
  printf("%s %lu %d\\n", tc->name, seeded, rand());
}
/* First, so only what the runner printed can be buffered when it forks. */
void Test_child(CuTest *tc)
{
  int status = 0;
  pid_t child;

  /* A child that flushes its copy of stdout must not repeat the seed line. */
  child = fork();
  if (child == 0)
    exit(0);
  CuAssertIntEquals(tc, child, waitpid(child, &status, 0));
  child = fork();
  if (child == 0)
    CuTestChildExit(7);
  CuAssertIntEquals(tc, child, waitpid(child, &status, 0));
  CuAssertTrue(tc, WIFEXITED(status) && WEXITSTATUS(status) == 7);
  report(tc);
}
void Test_alpha(CuTest *tc) { report(tc); }
void Test_beta(CuTest *tc) { report(tc); }
void Test_fails(CuTest *tc) { report(tc); CuFail(tc, "seeded failure"); }
""",
                "-DLUMINARI_CUTEST",
            )

            def draws(result):
                values = {}
                for line in result.stdout.splitlines():
                    fields = line.split()
                    if len(fields) == 3 and fields[0].startswith("Test_"):
                        values[fields[0]] = (int(fields[1]), int(fields[2]))
                return values

            default = run(executable)
            self.assertEqual(default.stdout.count("LUMINARI_TEST_SEED=1\n"), 1, default.stdout)
            self.assertIn("Runs: 4 Passes: 3 Fails: 1", default.stdout)
            self.assertIn(
                "Replay a failure with LUMINARI_TEST_SEED=1 CUTEST_FILTER=<test> ./cutest",
                default.stdout,
            )
            first = draws(default)
            self.assertEqual(sorted(first), ["Test_alpha", "Test_beta", "Test_child", "Test_fails"])
            self.assertEqual(len({seed for seed, _ in first.values()}), 4, first)
            for seed, _ in first.values():
                self.assertTrue(1 <= seed <= 2147483646, first)
            self.assertEqual(draws(run(executable)), first)
            self.assertEqual(draws(run(executable, LUMINARI_TEST_SEED="")), first)

            other = run(executable, LUMINARI_TEST_SEED="3000000000")
            self.assertIn("LUMINARI_TEST_SEED=3000000000\n", other.stdout)
            seeded = draws(other)
            self.assertTrue(all(seeded[name] != first[name] for name in first), seeded)
            # A filtered replay draws what the same test drew in the full run.
            replay = run(executable, LUMINARI_TEST_SEED="3000000000", CUTEST_FILTER="beta")
            self.assertEqual(draws(replay), {"Test_beta": seeded["Test_beta"]})

            for text in ("abc", "-1", "12x", " 5", "99999999999999999999999"):
                rejected = run(executable, LUMINARI_TEST_SEED=text)
                self.assertNotEqual(rejected.returncode, 0, text)
                self.assertIn("LUMINARI_TEST_SEED must be a decimal integer", rejected.stdout)
                self.assertEqual(draws(rejected), {}, text)

    def test_no_test_reseeds_from_the_clock(self):
        offenders = []
        for path in sorted(CUTEST.glob("*.c")):
            for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
                if CLOCK_RESEED.search(line):
                    offenders.append(f"{path.relative_to(ROOT)}:{number}: {line.strip()}")
        self.assertEqual(
            offenders,
            [],
            "the runner seeds every test from LUMINARI_TEST_SEED; do not reseed from the clock",
        )
        self.assertTrue(CLOCK_RESEED.search("  circle_srandom((unsigned long)time(NULL));"))
        self.assertIsNone(CLOCK_RESEED.search("  circle_srandom(1234);"))


if __name__ == "__main__":
    unittest.main()
