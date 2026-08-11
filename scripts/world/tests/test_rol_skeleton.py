from __future__ import annotations

import hashlib
from pathlib import Path
import tempfile
import unittest

from wtool_lib.rol_skeleton import RolSkeletonError, apply_keep_action, tree_manifest


def sha256(path: Path) -> str:
  return hashlib.sha256(path.read_bytes()).hexdigest()


class RolSkeletonTests(unittest.TestCase):
  def test_tree_manifest_is_stable_and_detects_content_changes(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      root = Path(temporary)
      (root / "wld").mkdir()
      path = root / "wld/1.wld"
      path.write_text("#1\nFirst~\n$\n", encoding="ascii")

      first = tree_manifest(root)
      second = tree_manifest(root)
      self.assertEqual(first, second)

      path.write_text("#1\nChanged~\n$\n", encoding="ascii")
      self.assertNotEqual(first["tree_sha256"], tree_manifest(root)["tree_sha256"])

  def test_keep_action_is_a_repeatable_zero_write_apply(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      root = Path(temporary)
      (root / "zon").mkdir()
      target = root / "zon/1960.zon"
      target.write_text("#1960\nJotun~\n$\n", encoding="ascii")
      expected = sha256(target)
      action = {
          "action": "KEEP",
          "selected_target": {
              "path": "zon/1960.zon",
              "source_file_sha256": expected,
          },
      }

      before = tree_manifest(root)
      first = apply_keep_action(action, root)
      second = apply_keep_action(action, root)
      after = tree_manifest(root)

      self.assertTrue(first["no_op"])
      self.assertTrue(second["no_op"])
      self.assertEqual(0, first["writes"])
      self.assertEqual(before, after)

  def test_keep_action_rejects_unsafe_target_path(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      action = {
          "action": "KEEP",
          "selected_target": {
              "path": "../outside.zon",
              "source_file_sha256": "0" * 64,
          },
      }
      with self.assertRaises(RolSkeletonError):
        apply_keep_action(action, Path(temporary))


if __name__ == "__main__":
  unittest.main()
