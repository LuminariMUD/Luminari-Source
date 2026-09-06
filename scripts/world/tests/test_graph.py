from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from tests.test_cli import make_world, tree_hash
from wtool_lib.constants import default_repo_root, load_manifest
from wtool_lib.world import (
    load_indexed_world_data,
    validate_explicit_paths,
    validate_indexed_world,
)


def quest_with_references(
    vnum: int,
    quest_type: int,
    target: int,
    questmaster: int = 100,
    previous: int = -1,
    next_quest: int = -1,
    prerequisite: int = -1,
    return_mobile: int = -1,
    reward_object: int = -1,
    follower_mobile: int = -1,
    alternative: int = -1,
) -> str:
  return (
      f"#{vnum}\nQuest {vnum}~\nA graph quest.~\nAccepted.~\nCompleted.~\nQuit.~\n"
      f"{quest_type} {questmaster} 0 {target} {previous} {next_quest} {prerequisite}\n"
      f"1 0 1 30 -1 {return_mobile} 1\n"
      f"0 0 {reward_object} -1 0 0 {follower_mobile}\n"
      f"D\n-1 -1 -1 {alternative}\nS\n"
  )


class FullGraphTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.repo_root = default_repo_root()
    cls.manifest = load_manifest()
    cls.config = {"diagonal_dirs": False, "config_source": "test", "assumed": False}
    cls.quest_types = {
        entry["macro"]: entry["index"]
        for entry in cls.manifest["tables"]["quest-types"]["entries"]
        if entry.get("macro")
    }

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

  def test_tail_reset_accepts_runtime_ring_eligibility(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      obj = root / "obj/1.obj"
      obj.write_text(
          obj.read_text(encoding="ascii").replace(
              "18 0 0 0 0 1 0 0 0", "11 0 0 0 0 ab 0 0 0"
          ),
          encoding="ascii",
      )
      zone = root / "zon/1.zon"
      zone.write_text(
          zone.read_text(encoding="ascii").replace(
              "S\n$\n",
              "M 0 100 1 100 100\nE 0 100 1 43 100\nS\n$\n",
          ),
          encoding="ascii",
      )

      self.assertNotIn("REF030", {item.code for item in self.validate(root).findings})

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

  def test_qst_reference_model_covers_every_edge_role_and_target_type(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      qst = root / "qst/1.qst"
      qst.write_text(
          quest_with_references(
              100,
              self.quest_types["AQ_OBJ_RETURN"],
              100,
              previous=101,
              next_quest=102,
              prerequisite=100,
              return_mobile=100,
              reward_object=100,
              follower_mobile=100,
          )
          + quest_with_references(101, self.quest_types["AQ_ROOM_FIND"], 100)
          + quest_with_references(102, self.quest_types["AQ_MOB_FIND"], 100)
          + quest_with_references(
              103,
              self.quest_types["AQ_DIALOGUE"],
              100,
              alternative=102,
          )
          + "$~\n",
          encoding="ascii",
      )
      world = load_indexed_world_data(
          root, self.repo_root, self.manifest, self.config
      )
      references = [reference for record in world.quests for reference in record.references]
      roles = {reference.role for reference in references}
      self.assertEqual(
          {
              "questmaster mobile",
              "type-sensitive quest target",
              "quest return recipient",
              "quest prerequisite object",
              "quest reward object",
              "quest follower reward",
              "previous quest",
              "next quest",
              "dialogue alternative quest",
          },
          roles,
      )
      target_types = {
          reference.target_type
          for reference in references
          if reference.role == "type-sensitive quest target"
      }
      self.assertEqual({"mobile", "object", "room"}, target_types)
      self.assertFalse(any(item.code.startswith("REF03") for item in world.findings))

  def test_qst_missing_and_wrong_type_references_are_source_located(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      qst = root / "qst/1.qst"
      qst.write_text(
          quest_with_references(
              100,
              self.quest_types["AQ_OBJ_RETURN"],
              999,
              questmaster=999,
              previous=999,
              next_quest=999,
              prerequisite=999,
              return_mobile=999,
              reward_object=999,
              follower_mobile=999,
              alternative=999,
          )
          + "$~\n",
          encoding="ascii",
      )
      missing = [item for item in self.validate(root).findings if item.code == "REF032"]
      self.assertEqual(9, len(missing))
      self.assertTrue(all(item.span.path == "qst/1.qst" for item in missing))
      self.assertTrue(all(item.span.column > 1 for item in missing))

      room_file = root / "wld/1.wld"
      room_file.write_text(
          room_file.read_text(encoding="ascii").replace(
              "$~\n",
              "#101\nWrong Type~\nA room-only target.~\n1 0 0 0 0 0\nS\n$~\n",
          ),
          encoding="ascii",
      )
      qst.write_text(
          quest_with_references(100, self.quest_types["AQ_MOB_FIND"], 101) + "$~\n",
          encoding="ascii",
      )
      wrong = [item for item in self.validate(root).findings if item.code == "REF033"]
      self.assertEqual(1, len(wrong))
      self.assertIsNotNone(wrong[0].related)
      self.assertEqual("room", wrong[0].related.record_type)

  def test_hlq_reference_model_and_missing_targets_cover_nested_context(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      hlq = root / "hlq/1.hlq"
      hlq.write_text(
          "#100\nR!\n100\nRoom reply.~\n"
          "I I 100 0\nO I 100 0\nO O 100 100\nO M 100 100\nO X 0 100\nS\n$~\n",
          encoding="ascii",
      )
      world = load_indexed_world_data(
          root, self.repo_root, self.manifest, self.config
      )
      references = world.hlquests[0].references
      self.assertEqual(9, len(references))
      self.assertEqual(
          {"mobile", "object", "room"},
          {reference.target_type for reference in references},
      )
      self.assertTrue(any("entry 1 input command 1 item" in ref.role for ref in references))
      self.assertFalse(any(item.code.startswith("REF03") for item in world.findings))

      hlq.write_text(
          "#999\nR!\n999\nRoom reply.~\n"
          "I I 999 0\nO I 999 0\nO O 999 999\nO M 999 999\nO X 0 999\nS\n$~\n",
          encoding="ascii",
      )
      missing = [item for item in self.validate(root).findings if item.code == "REF034"]
      self.assertEqual(9, len(missing))
      self.assertTrue(all("entry 1" in item.message or "host" in item.message for item in missing))

  def test_quest_reference_findings_respect_selected_packages(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      (root / "qst/1.qst").write_text(
          quest_with_references(100, self.quest_types["AQ_MOB_FIND"], 999) + "$~\n",
          encoding="ascii",
      )
      (root / "zon/2.zon").write_text(
          "#2\nBuilder~\nSelected graph zone~\n200 299 30 2\nS\n$\n",
          encoding="ascii",
      )
      (root / "qst/2.qst").write_text(
          quest_with_references(200, self.quest_types["AQ_MOB_FIND"], 100) + "$~\n",
          encoding="ascii",
      )
      result = validate_indexed_world(
          root,
          self.repo_root,
          self.manifest,
          self.config,
          selected_packages={2},
      )
      self.assertNotIn("REF032", {item.code for item in result.findings})


if __name__ == "__main__":
  unittest.main()
