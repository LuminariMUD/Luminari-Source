from __future__ import annotations

from contextlib import redirect_stderr, redirect_stdout
import hashlib
from io import StringIO
import json
from pathlib import Path
import unittest

from wtool_lib.cli import main
from wtool_lib.constants import default_repo_root
from wtool_lib.rol_inventory import (
    RolInventoryError,
    SOURCE_KINDS,
    build_rol_inventory,
    parse_rol_manifest,
    render_rol_inventory_human,
)


def tree_hash(root: Path) -> str:
  digest = hashlib.sha256()
  for path in sorted(item for item in root.rglob("*") if item.is_file()):
    digest.update(path.relative_to(root).as_posix().encode("ascii"))
    digest.update(path.read_bytes())
  return digest.hexdigest()


class RolInventoryTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.repo_root = default_repo_root()
    cls.fixture_root = (
        cls.repo_root / "scripts/world/tests/fixtures/rol_inventory/valid"
    )

  def test_fixture_enumerates_seven_kinds_and_all_classifications(self) -> None:
    inventory = build_rol_inventory(self.fixture_root, self.repo_root)
    zones = inventory["summary"]["zones"]
    self.assertEqual(list(SOURCE_KINDS), inventory["source_kinds"])
    self.assertEqual(2, zones["active_files"])
    self.assertEqual(3, zones["active_records"])
    self.assertEqual(1, zones["disabled_files"])
    self.assertEqual(1, zones["unlisted_files"])
    self.assertEqual(1, zones["active_basenames_without_zone"])
    self.assertEqual(1, zones["active_multi_zone_files"])
    self.assertEqual(2, zones["active_companion_only_files"])
    self.assertEqual(
        ["companion"],
        inventory["classifications"]["active_basenames_without_zone"],
    )
    self.assertEqual(
        [{"basename": "split", "record_vnums": [200, 201]}],
        inventory["classifications"]["active_multi_zone_files"],
    )

    files = {(item["kind"], item["basename"]): item for item in inventory["files"]}
    self.assertTrue(files[("obj", "companion")]["included"])
    self.assertEqual("active", files[("qst", "companion")]["status"])
    self.assertFalse(files[("zon", "disabled")]["included"])
    self.assertEqual("disabled", files[("zon", "disabled")]["status"])
    self.assertFalse(files[("zon", "orphan")]["included"])
    self.assertEqual("unlisted", files[("zon", "orphan")]["status"])

  def test_json_and_human_outputs_are_repeatable(self) -> None:
    first = build_rol_inventory(self.fixture_root, self.repo_root)
    second = build_rol_inventory(self.fixture_root, self.repo_root)
    self.assertEqual(first, second)
    self.assertEqual(
        json.dumps(first, ensure_ascii=True, indent=2, sort_keys=True),
        json.dumps(second, ensure_ascii=True, indent=2, sort_keys=True),
    )
    human = render_rol_inventory_human(first)
    self.assertEqual(human, render_rol_inventory_human(second))
    self.assertIn("Active zone scope: 2 files, 3 records", human)
    self.assertIn("Excluded zone files: 1 disabled, 1 unlisted", human)

  def test_malformed_manifest_entries_report_each_source_line(self) -> None:
    path = self.repo_root / "scripts/world/tests/fixtures/rol_inventory/malformed/areas/AREA"
    with self.assertRaises(RolInventoryError) as caught:
      parse_rol_manifest(path, "areas/AREA")
    message = str(caught.exception)
    self.assertIn("areas/AREA:2", message)
    self.assertIn("not a safe path component", message)
    self.assertIn("areas/AREA:3", message)
    self.assertIn("does not contain a basename", message)
    self.assertIn("areas/AREA:4", message)
    self.assertIn("'*not-a-column-zero-comment'", message)

  def test_cli_emits_clean_json_and_explicit_operational_errors(self) -> None:
    before = tree_hash(self.fixture_root)
    stdout = StringIO()
    stderr = StringIO()
    with redirect_stdout(stdout), redirect_stderr(stderr):
      status = main(
          ["--json", "rol-inventory", "--source-root", str(self.fixture_root)]
      )
    self.assertEqual(0, status)
    self.assertEqual("", stderr.getvalue())
    self.assertEqual(3, json.loads(stdout.getvalue())["summary"]["zones"]["active_records"])
    first_json = stdout.getvalue()

    stdout = StringIO()
    stderr = StringIO()
    with redirect_stdout(stdout), redirect_stderr(stderr):
      status = main(
          ["--json", "rol-inventory", "--source-root", str(self.fixture_root)]
      )
    self.assertEqual(0, status)
    self.assertEqual("", stderr.getvalue())
    self.assertEqual(first_json, stdout.getvalue())
    self.assertEqual(before, tree_hash(self.fixture_root))

    malformed = self.repo_root / "scripts/world/tests/fixtures/rol_inventory/malformed"
    stdout = StringIO()
    stderr = StringIO()
    with redirect_stdout(stdout), redirect_stderr(stderr):
      status = main(["rol-inventory", "--source-root", str(malformed)])
    self.assertEqual(2, status)
    self.assertEqual("", stdout.getvalue())
    self.assertIn("malformed RoL manifest entries", stderr.getvalue())
    self.assertIn("areas/AREA:2", stderr.getvalue())

  def test_ignored_reference_corpus_matches_locked_scope(self) -> None:
    source_root = self.repo_root / "EXAMPLE/RealmsOfLuminari"
    if not source_root.is_dir():
      self.skipTest("ignored RealmsOfLuminari reference corpus is not installed")
    inventory = build_rol_inventory(source_root, self.repo_root)
    zones = inventory["summary"]["zones"]
    self.assertEqual(252, zones["active_files"])
    self.assertEqual(255, zones["active_records"])
    self.assertEqual(30, zones["disabled_files"])
    self.assertEqual(2, zones["unlisted_files"])
    self.assertEqual(6, zones["active_basenames_without_zone"])
    self.assertEqual(2, zones["active_multi_zone_files"])
    self.assertEqual(9, zones["active_companion_only_files"])
    self.assertEqual(
        [
            "foggy_woods2",
            "god_items",
            "northern_highroad2",
            "northern_highroad3",
            "quest_1",
            "quest_2",
        ],
        inventory["classifications"]["active_basenames_without_zone"],
    )


if __name__ == "__main__":
  unittest.main()
