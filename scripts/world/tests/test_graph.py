from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from tests.test_cli import make_world, tree_hash
from wtool_lib.constants import default_repo_root, load_manifest
from wtool_lib.world import validate_indexed_world


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
      self.assertEqual([], self.validate(root).findings)

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
    self.assertEqual({"OBJ027", "ZON033"}, {item.code for item in normal.findings})
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


if __name__ == "__main__":
  unittest.main()
