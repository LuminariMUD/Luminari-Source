from __future__ import annotations

import hashlib
import json
from pathlib import Path
import shutil
import tempfile
import unittest

from wtool_lib.constants import default_repo_root
from wtool_lib.rol_baseline import (
    RolBaselineError,
    _source_build_bytes,
    build_target_inventory,
    load_rol_policy,
    reconcile_source_aggregates,
)
from wtool_lib.rol_inventory import build_rol_inventory


class RolBaselineTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.repo_root = default_repo_root()
    cls.source_fixture = cls.repo_root / "scripts/world/tests/fixtures/rol_inventory/valid"

  def test_source_build_bytes_matches_unterminated_c_reader_behavior(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      path = Path(temporary) / "sample.obj"
      path.write_bytes(b"#1\nbody\nunterminated")
      data, dropped = _source_build_bytes(path, "obj")
      self.assertEqual(b"#1\nbody\n", data)
      self.assertEqual(b"unterminated", dropped)

      path.write_bytes(b"<*>\n#1\nbody\n")
      data, dropped = _source_build_bytes(path, "wld")
      self.assertEqual(b"#1\nbody\n", data)
      self.assertIsNone(dropped)

  def test_aggregate_reconciliation_uses_active_manifest_order(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      source_root = Path(temporary) / "source"
      shutil.copytree(self.source_fixture, source_root)
      inventory = build_rol_inventory(source_root, self.repo_root)
      for kind in inventory["source_kinds"]:
        chunks = []
        manifest_name = {
            "zon": "AREA",
            "wld": "AREA",
            "soc": "AREA",
            "mob": "AREA.mobobj",
            "obj": "AREA.mobobj",
            "shp": "SHOP",
            "qst": "QUEST",
        }[kind]
        manifest = next(
            item for item in inventory["manifests"] if item["name"] == manifest_name
        )
        for entry in manifest["active"]:
          path = source_root / "areas" / kind / f"{entry['basename']}.{kind}"
          if path.is_file():
            data, _ = _source_build_bytes(path, kind)
            chunks.append(data)
        (source_root / "areas" / f"world.{kind}").write_bytes(b"".join(chunks))

      result = reconcile_source_aggregates(source_root, inventory)
      self.assertTrue(result["all_byte_identical"])
      self.assertTrue(result["complete"])
      self.assertEqual(7, len(result["kinds"]))

  def test_target_inventory_classifies_indexed_missing_and_orphan_files(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      world_root = Path(temporary) / "world"
      for kind in ("zon", "wld", "mob", "obj", "shp", "trg", "qst", "hlq"):
        directory = world_root / kind
        directory.mkdir(parents=True)
        (directory / "index").write_text(f"1.{kind}\n2.{kind}\n$\n", encoding="ascii")
        (directory / "index.mini").write_text("$\n", encoding="ascii")
        (directory / f"1.{kind}").write_text("#1\n$\n", encoding="ascii")
        (directory / f"3.{kind}").write_text("#3\n$\n", encoding="ascii")

      inventory = build_target_inventory(world_root, self.repo_root)
      self.assertEqual(16, inventory["summary"]["files"])
      self.assertEqual(8, inventory["summary"]["indexed"])
      self.assertEqual(8, inventory["summary"]["missing"])
      self.assertEqual(8, inventory["summary"]["orphaned"])
      first = json.dumps(inventory, ensure_ascii=True, indent=2, sort_keys=True)
      second = json.dumps(
          build_target_inventory(world_root, self.repo_root),
          ensure_ascii=True,
          indent=2,
          sort_keys=True,
      )
      self.assertEqual(hashlib.sha256(first.encode()).digest(), hashlib.sha256(second.encode()).digest())

  def test_versioned_policy_is_ascii_and_locked(self) -> None:
    policy = load_rol_policy(self.repo_root)
    self.assertEqual("rol-conversion-policy-1", policy["policy_version"])
    self.assertEqual("!", policy["quest"]["approval_marker"])
    self.assertFalse(policy["apply"]["implicit_overwrite"])

  def test_missing_policy_is_an_operational_error(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      with self.assertRaises(RolBaselineError):
        load_rol_policy(Path(temporary))


if __name__ == "__main__":
  unittest.main()
