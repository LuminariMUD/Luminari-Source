from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from tests.test_cli import make_world, tree_hash
from wtool_lib.constants import default_repo_root, load_manifest
from wtool_lib.world import validate_explicit_paths, validate_indexed_world


class FullGraphTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.repo_root = default_repo_root()
    cls.manifest = load_manifest()
    cls.config = {"diagonal_dirs": False, "config_source": "test", "assumed": False}

  def validate(self, root: Path):
    return validate_indexed_world(root, self.repo_root, self.manifest, self.config)

  def test_complete_six_type_graph_is_clean(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      findings = self.validate(root).findings
      self.assertEqual([], [item for item in findings if item.severity == "error"])
      self.assertEqual({"SEM005", "SEM010"}, {item.code for item in findings})

  def test_tracked_complete_fixture_covers_normal_mini_and_legacy_read_only(self) -> None:
    root = self.repo_root / "scripts/world/tests/fixtures/phase2/complete"
    before = tree_hash(root)
    normal = self.validate(root)
    mini = validate_indexed_world(
        root, self.repo_root, self.manifest, self.config, mini=True
    )
    self.assertTrue(normal.complete)
    self.assertTrue(mini.complete)
    self.assertEqual([], [item for item in normal.findings if item.severity == "error"])
    self.assertEqual(
        {"OBJ027", "SEM010", "ZON033"},
        {item.code for item in normal.findings},
    )
    self.assertEqual(before, tree_hash(root))

  def test_tracked_real_bundles_have_stable_phase2_results_and_are_read_only(self) -> None:
    expected_errors = {"artifacts": set(), "minimal": {"MOB016"}}
    for name, expected in expected_errors.items():
      with self.subTest(bundle=name):
        root = self.repo_root / "lib/world" / name
        before = tree_hash(root)
        result = validate_explicit_paths([root], self.repo_root, self.manifest, self.config)
        self.assertEqual(expected, {item.code for item in result.findings if item.severity == "error"})
        self.assertEqual(before, tree_hash(root))

  def test_missing_and_wrong_type_references_are_distinct(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      shop = root / "shp/1.shp"
      original = shop.read_text(encoding="ascii")
      shop.write_text(original.replace("#100~\n100\n", "#100~\n999\n"), encoding="ascii")
      self.assertIn("REF022", {item.code for item in self.validate(root).findings})

      shop.write_text(original.replace("#100~\n100\n", "#100~\n101\n"), encoding="ascii")
      room = root / "wld/1.wld"
      room.write_text(
          room.read_text(encoding="ascii").replace(
              "$~\n", "#101\nOther Room~\nAnother room.~\n1 0 0 0 0 0\nS\n$~\n"
          ),
          encoding="ascii",
      )
      self.assertIn("REF023", {item.code for item in self.validate(root).findings})

  def test_trigger_attach_type_and_key_item_type_are_checked(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      mobile = root / "mob/1.mob"
      mobile.write_text(
          mobile.read_text(encoding="ascii").replace("$\n", "T 100\n$\n"),
          encoding="ascii",
      )
      room = root / "wld/1.wld"
      room.write_text(
          "#100\nTest Room~\nA test room.~\n1 0 0 0 0 0\nD0\n~\nkey~\n1 100 100\nS\n$~\n",
          encoding="ascii",
      )
      obj = root / "obj/1.obj"
      obj.write_text(obj.read_text(encoding="ascii").replace("18 0 0 0 0", "5 0 0 0 0"), encoding="ascii")
      codes = {item.code for item in self.validate(root).findings}
      self.assertTrue({"REF021", "REF025"} <= codes)

  def test_reset_prototypes_and_e_wear_compatibility_are_checked(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      zone = root / "zon/1.zon"
      zone.write_text(
          zone.read_text(encoding="ascii").replace(
              "S\n$\n",
              "M 0 100 1 100 100\nE 0 100 1 16 100\nT 0 2 100 100\nS\n$\n",
          ),
          encoding="ascii",
      )
      codes = {item.code for item in self.validate(root).findings}
      self.assertIn("REF030", codes)
      self.assertNotIn("REF022", codes)

  def test_p_reset_requires_a_container_type(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      zone = root / "zon/1.zon"
      zone.write_text(
          zone.read_text(encoding="ascii").replace(
              "S\n$\n", "P 0 100 1 100 100\nS\n$\n"
          ),
          encoding="ascii",
      )
      self.assertIn("REF031", {item.code for item in self.validate(root).findings})

  def test_container_keys_require_key_prototypes(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      obj = root / "obj/1.obj"
      content = obj.read_text(encoding="ascii")
      content = content.replace("18 0 0 0 0", "15 0 0 0 0")
      content = content.replace(
          "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
          "10 0 100 0 0 0 0 0 0 0 0 0 0 0 0 0",
      )
      obj.write_text(content, encoding="ascii")
      self.assertIn("REF025", {item.code for item in self.validate(root).findings})

  def test_reference_roles_report_wrong_type_targets(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      room = root / "wld/1.wld"
      room.write_text(
          room.read_text(encoding="ascii").replace(
              "$~\n", "#101\nWrong Type~\nA room-only vnum.~\n1 0 0 0 0 0\nS\n$~\n"
          ),
          encoding="ascii",
      )
      zone = root / "zon/1.zon"
      zone.write_text(
          zone.read_text(encoding="ascii").replace(
              "S\n$\n",
              "M 0 101 1 100 100\nO 0 101 1 100 100\nT 0 2 101 100\nS\n$\n",
          ),
          encoding="ascii",
      )
      obj = root / "obj/1.obj"
      obj.write_text(
          obj.read_text(encoding="ascii").replace(
              "$\n", "J\n101\nC\n53 10 4 101 0 0 0 summon\n$\n"
          ),
          encoding="ascii",
      )
      shop = root / "shp/1.shp"
      shop.write_text(
          shop.read_text(encoding="ascii")
          .replace("#100~\n100\n", "#100~\n101\n")
          .replace("0\n0\n100\n0\n100\n-1\n", "0\n0\n101\n0\n100\n-1\n"),
          encoding="ascii",
      )
      messages = [item.message for item in self.validate(root).findings if item.code == "REF023"]
      for role in (
          "M reset prototype",
          "O reset prototype",
          "T reset trigger",
          "object recipient",
          "object special ability summon",
          "shop product",
          "shop keeper",
      ):
        self.assertTrue(any(role in message for message in messages), role)


if __name__ == "__main__":
  unittest.main()
