from __future__ import annotations

import json
from pathlib import Path
import subprocess
import tempfile
import unittest

from tests.test_cli import make_world, tree_hash
from wtool_lib.constants import default_repo_root


class ZoneWrapperTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.wrapper = default_repo_root() / "lib/world/validate-zone.sh"

  def run_wrapper(
      self,
      cwd: Path,
      world_root: Path,
      zone: str,
      *options: str,
  ) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [
            str(self.wrapper),
            zone,
            "--world-root",
            str(world_root),
            *options,
        ],
        cwd=cwd,
        check=False,
        capture_output=True,
        text=True,
    )

  def test_wrapper_finds_repo_outside_worktree_and_is_read_only(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      temp_root = Path(directory)
      world_root = temp_root / "world"
      outside = temp_root / "outside"
      outside.mkdir()
      make_world(world_root)
      before = tree_hash(world_root)
      completed = self.run_wrapper(outside, world_root, "1", "--json")
      self.assertEqual(0, completed.returncode, completed.stderr)
      self.assertEqual("", completed.stderr)
      self.assertEqual("zone", json.loads(completed.stdout)["mode"])
      self.assertEqual(before, tree_hash(world_root))

  def test_wrapper_preserves_data_and_operational_exit_statuses(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      temp_root = Path(directory)
      world_root = temp_root / "world"
      outside = temp_root / "outside"
      outside.mkdir()
      make_world(world_root)
      data_error = self.run_wrapper(outside, world_root, "999", "--json")
      operational_error = self.run_wrapper(outside, temp_root / "missing", "1")
      strict_warning = self.run_wrapper(
          outside,
          default_repo_root() / "scripts/world/tests/fixtures/phase2/complete",
          "100",
          "--strict",
      )
      self.assertEqual(1, data_error.returncode)
      self.assertEqual(2, operational_error.returncode)
      self.assertEqual(1, strict_warning.returncode)

  def test_wrapper_without_zone_keeps_legacy_usage_status(self) -> None:
    completed = subprocess.run(
        [str(self.wrapper)],
        cwd=default_repo_root().parent,
        check=False,
        capture_output=True,
        text=True,
    )
    self.assertEqual(1, completed.returncode)
    self.assertIn("Usage:", completed.stderr)


if __name__ == "__main__":
  unittest.main()
