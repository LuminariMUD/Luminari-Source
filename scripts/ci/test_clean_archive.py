#!/usr/bin/env python3
"""Exercise archive runtime handoff without compiling the game or opening a database.

Use a real temporary Git repository and the production archive script. Only
the build tools are replaced; they record what reaches make test and ctest.
The full clean-archive CI job remains the boot and MariaDB integration gate.
"""

import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
BUILD_TOOL = """#!/usr/bin/env python3
import json
import os
from pathlib import Path
import sys

tool = Path(sys.argv[0]).name
args = sys.argv[1:]
with open(os.environ['ARCHIVE_TEST_RECORD'] + '.calls', 'a') as calls:
    calls.write(json.dumps([tool, *args]) + '\\n')
if tool == 'autoreconf':
    Path('configure').write_text('#!/bin/sh\\nexit 0\\n')
    Path('configure').chmod(0o755)
if (tool == 'make' and 'test' in args) or tool == 'ctest':
    config = os.environ.get('LUMINARI_TEST_CONFIG_FILE')
    record = {
        'tool': tool,
        'root': str(Path.cwd()),
        'environment': {key: value for key, value in os.environ.items()
                        if key.startswith('LUMINARI_TEST_')},
        'config': Path(config).read_text() if config else None,
        'untracked_present': Path('untracked-sentinel').exists(),
    }
    with open(os.environ['ARCHIVE_TEST_RECORD'], 'a') as output:
        output.write(json.dumps(record) + '\\n')
if ((tool == 'make' and 'install' in args)
        or (tool == 'cmake' and '--install' in args)):
    Path('bin').mkdir(exist_ok=True)
    Path('bin/luminari').write_text('#!/bin/sh\\nexit 0\\n')
    Path('bin/luminari').chmod(0o755)
"""


class CleanArchiveTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="test-clean-archive-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name).resolve()
        self.repo = self.root / "source checkout"
        self.repo.mkdir()
        script = self.repo / "scripts/ci/check_clean_archive.sh"
        script.parent.mkdir(parents=True)
        shutil.copy2(ROOT / "scripts/ci/check_clean_archive.sh", script)
        (self.repo / "src" / "config").mkdir(parents=True)
        for name in ("campaign", "mud_options", "vnums"):
            (self.repo / f"src/config/{name}.example.h").write_text("/* fixture */\n")
        self.git("init", "--quiet")
        self.git("add", ".")
        self.git("-c", "user.name=Test Fixture", "-c", "user.email=test@example.invalid",
                 "-c", "core.hooksPath=/dev/null", "-c", "commit.gpgsign=false",
                 "commit", "--quiet", "-m", "Archive test fixture")
        (self.repo / "untracked-sentinel").touch()

        self.tools = self.root / "tools"
        self.tools.mkdir()
        for name in ("autoreconf", "make", "cmake", "ctest"):
            executable = self.tools / name
            executable.write_text(BUILD_TOOL)
            executable.chmod(0o755)
        self.record = self.root / "test-entry-points.jsonl"
        self.env = {key: value for key, value in os.environ.items()
                    if not key.startswith(("LUMINARI_TEST_", "GIT_"))}
        self.env.update(PATH=f"{self.tools}:{os.environ['PATH']}",
                        TMPDIR=str(self.root), ARCHIVE_TEST_RECORD=str(self.record))

    def git(self, *args):
        subprocess.run(["git", "-C", str(self.repo), *args], check=True,
                       capture_output=True,
                       env={key: value for key, value in os.environ.items()
                            if not key.startswith("GIT_")})

    def run_archive(self):
        return subprocess.run(["bash", "scripts/ci/check_clean_archive.sh"],
                              cwd=self.repo, env=self.env, text=True, capture_output=True)

    def records(self):
        return [json.loads(line) for line in self.record.read_text().splitlines()]

    def test_external_runtime_reaches_both_test_entry_points(self):
        runtime = self.repo / "isolated runtime/lib"
        runtime.mkdir(parents=True)
        config = runtime / "config"
        config_text = "mortal_start_room = 3001\nimmort_start_room = 3002\n"
        config.write_text(config_text)
        database = {
            "LUMINARI_TEST_MYSQL_ENABLE": "1",
            "LUMINARI_TEST_MYSQL_HOST": "127.0.0.1",
            "LUMINARI_TEST_MYSQL_USER": "archive_test",
            "LUMINARI_TEST_MYSQL_PASSWORD": "fixture-only",
            "LUMINARI_TEST_MYSQL_DATABASE": "archive_test",
            "LUMINARI_TEST_MYSQL_PORT": "3306",
        }
        self.env.update(database, LUMINARI_TEST_DATA_DIR=str(runtime))
        # CI supplies an absolute path; developers can supply a checkout-relative one.
        for path in (str(config), str(config.relative_to(self.repo))):
            with self.subTest(config_path=path):
                self.record.unlink(missing_ok=True)
                self.env["LUMINARI_TEST_CONFIG_FILE"] = path
                result = self.run_archive()
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                records = self.records()
                self.assertEqual([record["tool"] for record in records], ["make", "ctest"])
                for record in records:
                    environment = record["environment"]
                    # Exact match: the server also rejects "./" and "//" forms of this path.
                    self.assertEqual(environment["LUMINARI_TEST_CONFIG_FILE"], "lib/etc/config")
                    self.assertEqual(record["config"], config_text)
                    self.assertEqual(environment["LUMINARI_TEST_DATA_DIR"], str(runtime))
                    for key, value in database.items():
                        self.assertEqual(environment[key], value)
                    self.assertNotIn("LUMINARI_TEST_SKIP_SYNTAX_BOOT", environment)
                    self.assertEqual(environment["LUMINARI_TEST_ROOT"], record["root"])
                    self.assertEqual(environment["LUMINARI_TEST_SPEC_WORLD_ROOT"],
                                     str(Path(record["root"]) /
                                         "unittests/CuTest/fixtures/spec_world_inventory"))
                    self.assertNotEqual(record["root"], str(self.repo))
                    self.assertFalse(record["untracked_present"])
                    self.assertFalse(Path(record["root"]).exists())
                self.assertEqual(config.read_text(), config_text)
                self.assertFalse((self.repo / "lib/etc/config").exists())

    def test_explicit_no_runtime_mode_is_preserved(self):
        self.env["LUMINARI_TEST_SKIP_SYNTAX_BOOT"] = "1"
        result = self.run_archive()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        records = self.records()
        self.assertEqual([record["tool"] for record in records], ["make", "ctest"])
        for record in records:
            self.assertIsNone(record["config"])
            environment = record["environment"]
            self.assertNotIn("LUMINARI_TEST_CONFIG_FILE", environment)
            self.assertNotIn("LUMINARI_TEST_MYSQL_ENABLE", environment)
            self.assertEqual(environment["LUMINARI_TEST_SKIP_SYNTAX_BOOT"], "1")

    def test_missing_config_fails_before_build(self):
        self.env["LUMINARI_TEST_CONFIG_FILE"] = str(self.root / "missing-config")
        result = self.run_archive()
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(Path(f"{self.record}.calls").exists())
        self.assertEqual(list(self.root.glob("luminari-archive.*")), [])


if __name__ == "__main__":
    unittest.main()
