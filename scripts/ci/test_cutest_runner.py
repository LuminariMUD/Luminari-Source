#!/usr/bin/env python3
"""Exercise the generated CuTest runner, including filtering and failed-test timing."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
CUTEST = ROOT / 'unittests/CuTest'


class RunnerTest(unittest.TestCase):
    def test_generated_runner(self):
        with tempfile.TemporaryDirectory(prefix='cutest-runner-') as directory:
            root = Path(directory)
            fixture = root / 'tests.c'
            fixture.write_text('''#include "CuTest.h"
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
''', encoding='ascii')
            generated = subprocess.check_output(
                ['bash', str(CUTEST / 'make-tests.sh'), str(fixture)])
            (root / 'AllTests.c').write_bytes(generated)
            executable = root / 'runner'
            subprocess.run([*shlex.split(os.environ.get('CC', 'cc')), '-std=gnu2x',
                            '-Wall', '-Wextra', '-Werror', '-I', str(CUTEST),
                            str(fixture), str(root / 'AllTests.c'), str(CUTEST / 'CuTest.c'),
                            '-lm', '-o', str(executable)], check=True)
            env = dict(os.environ)
            env.pop('CUTEST_FILTER', None)

            def run(filter_value=None):
                selected = dict(env)
                if filter_value is not None:
                    selected['CUTEST_FILTER'] = filter_value
                return subprocess.run([str(executable)], env=selected, text=True,
                                      stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

            full = run()
            self.assertNotEqual(full.returncode, 0)
            self.assertIn('Runs: 3 Passes: 2 Fails: 1', full.stdout)
            self.assertNotIn('tests selected', full.stdout)
            self.assertIn('Slow test: Test_slow_failure (', full.stdout)
            self.assertNotIn('Slow test: Test_first', full.stdout)
            selected = run('first')
            self.assertEqual(selected.returncode, 0, selected.stdout)
            self.assertIn('OK (1 test)', selected.stdout)
            self.assertIn('CUTEST_FILTER=first: 1 of 3 tests selected', selected.stdout)
            self.assertNotIn('slow_failure', selected.stdout)
            missing = run('does_not_exist')
            self.assertNotEqual(missing.returncode, 0)
            self.assertIn('No tests matched CUTEST_FILTER=does_not_exist', missing.stdout)
            empty = run('')
            self.assertIn('Runs: 3 Passes: 2 Fails: 1', empty.stdout)
            self.assertNotIn('tests selected', empty.stdout)
            self.assertNotEqual(empty.returncode, 0)


if __name__ == '__main__':
    unittest.main()
