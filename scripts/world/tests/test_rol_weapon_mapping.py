from __future__ import annotations

from pathlib import Path
import json
import re
import tempfile
import unittest

from wtool_lib.constants import default_repo_root
from wtool_lib.rol_source import parse_active_rol_corpus, parse_rol_source_file
from wtool_lib.rol_transform import emit_object
from wtool_lib import rol_weapon_mapping as mapping


def _resolver(kind: str, vnum: int) -> int:
  return vnum + 2_000_000


class RolWeaponMappingTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.root = default_repo_root()
    cls.corpus = parse_active_rol_corpus(cls.root / "EXAMPLE/RealmsOfLuminari", cls.root)
    cls.weapons = [
        record
        for record in cls.corpus.records
        if record.kind == "obj"
        and record.values.get("item_type") == mapping.SOURCE_ITEM_TYPE_WEAPON
    ]

  def _source_record(self, data: bytes):
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    path = Path(temporary.name) / "sample.obj"
    path.write_bytes(data)
    records, corpus = parse_rol_source_file(path, "areas/obj/sample.obj", "obj", "sample")
    self.assertTrue(corpus.complete)
    return records[0]

  def _weapon(self, aliases: str, short: str, extra: int, values: str) -> object:
    body = (
        f"#900\n{aliases}~\n{short}~\n{short} is here.~\n~\n"
        f"5 {extra} 0\n{values}\n10 100 0\n"
    ).encode("ascii")
    return self._source_record(body)

  def test_weapon_type_table_matches_the_target_header(self) -> None:
    header = (self.root / "src/structs.h").read_text(encoding="utf-8", errors="ignore")
    declared = {
        int(value): name
        for name, value in re.findall(r"#define (WEAPON_TYPE_\w+) (\d+)", header)
    }
    self.assertEqual(declared, mapping.WEAPON_TYPE_NAMES)
    count = re.search(r"#define NUM_WEAPON_TYPES (\d+)", header)
    self.assertIsNotNone(count)
    self.assertEqual(int(count.group(1)), mapping.NUM_WEAPON_TYPES)

  def test_ranged_weapon_set_matches_load_weapons(self) -> None:
    source = (self.root / "src/combat/assign_wpn_armor.c").read_text(
        encoding="utf-8", errors="ignore"
    )
    calls = re.findall(r"setweapon\(\s*(WEAPON_TYPE_\w+)\s*,(.*?)\);", source, re.S)
    declared = {name for name, body in calls if "WEAPON_FLAG_RANGED" in body}
    self.assertEqual(declared, set(mapping.RANGED_WEAPON_TYPES))

  def test_every_active_source_weapon_resolves_to_a_defined_type(self) -> None:
    self.assertEqual(1319, len(self.weapons))
    for record in self.weapons:
      inference = mapping.infer_weapon_type(record)
      self.assertGreaterEqual(inference.weapon_type, 1, record.record_id)
      self.assertLess(inference.weapon_type, mapping.NUM_WEAPON_TYPES, record.record_id)
      self.assertNotIn(inference.name, mapping.RANGED_WEAPON_TYPES, record.record_id)
      self.assertIn(inference.tier, {"override", "keyword", "fallback"})

  def test_keyword_and_override_tiers_carry_the_corpus(self) -> None:
    report = mapping.audit(self.corpus.records)
    self.assertEqual(0, report["undefined"])
    self.assertEqual(1319, report["weapons"])
    # The fallback matrix is a safety net, not a workhorse. Every record it
    # still catches is a builder placeholder whose verb and handedness are the
    # only identity it was ever given.
    self.assertLessEqual(report["tiers"]["fallback"], 30)
    self.assertGreaterEqual(report["tiers"]["keyword"], 1200)

  def test_overrides_are_curated_against_records_that_exist(self) -> None:
    overrides = mapping.load_overrides()
    self.assertTrue(overrides)
    vnums = {record.vnum for record in self.weapons}
    for vnum, entry in overrides.items():
      self.assertIn(vnum, vnums, f"override {vnum} names no active source weapon")
      self.assertTrue(entry["rationale"], f"override {vnum} carries no rationale")

  def test_override_catalog_rejects_undefined_and_ranged_types(self) -> None:
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    path = Path(temporary.name) / "overrides.json"

    path.write_text(json.dumps({"overrides": {"1": {"weapon_type": "WEAPON_TYPE_UNDEFINED"}}}))
    with self.assertRaisesRegex(ValueError, "WEAPON_TYPE_UNDEFINED"):
      mapping.load_overrides(path)

    path.write_text(json.dumps({"overrides": {"1": {"weapon_type": "WEAPON_TYPE_LONG_BOW"}}}))
    with self.assertRaisesRegex(ValueError, "ranged weapon type"):
      mapping.load_overrides(path)

    path.write_text(json.dumps({"overrides": {"1": {"weapon_type": "WEAPON_TYPE_SPORK"}}}))
    with self.assertRaisesRegex(ValueError, "unknown weapon type"):
      mapping.load_overrides(path)

  def test_handedness_selects_between_paired_weapon_types(self) -> None:
    one_handed = self._weapon("sword long", "a long sword", 0, "0 1 8 3")
    two_handed = self._weapon("sword long", "a long sword", 1 << 22, "0 1 8 3")
    self.assertEqual(
        mapping.WEAPON_TYPE_IDS["WEAPON_TYPE_LONG_SWORD"],
        mapping.infer_weapon_type(one_handed).weapon_type,
    )
    self.assertEqual(
        mapping.WEAPON_TYPE_IDS["WEAPON_TYPE_GREAT_SWORD"],
        mapping.infer_weapon_type(two_handed).weapon_type,
    )

  def test_specific_names_outrank_the_general_nouns_they_contain(self) -> None:
    cases = [
        ("axe battle razor", "a razor edge battleaxe", 0, "WEAPON_TYPE_BATTLE_AXE"),
        ("pickaxe dwarven", "a dwarven pickaxe", 0, "WEAPON_TYPE_LIGHT_PICK"),
        ("glaive polearm", "a glaive polearm", 0, "WEAPON_TYPE_GLAIVE"),
        ("ranseur polearm iron", "a decorated iron ranseur", 0, "WEAPON_TYPE_RANSEUR"),
        ("scourge chain", "a chain scourge", 0, "WEAPON_TYPE_WHIP"),
        ("bastard sword", "a bastard sword", 0, "WEAPON_TYPE_BASTARD_SWORD"),
        ("shortsword gladius", "a gladius", 0, "WEAPON_TYPE_SHORT_SWORD"),
    ]
    for aliases, short, extra, expected in cases:
      with self.subTest(short=short):
        record = self._weapon(aliases, short, extra, "0 1 6 3")
        self.assertEqual(expected, mapping.infer_weapon_type(record).name)

  def test_weak_modifiers_never_outrank_a_named_weapon(self) -> None:
    decorated = self._weapon("spear hunting razor", "a razor-sharp hunting spear", 0, "0 1 6 11")
    bare = self._weapon("razor barber", "a sharp barber's razor", 0, "0 1 3 3")
    self.assertEqual("WEAPON_TYPE_SPEAR", mapping.infer_weapon_type(decorated).name)
    self.assertEqual("WEAPON_TYPE_KNIFE", mapping.infer_weapon_type(bare).name)

  def test_fallback_matrix_reads_the_source_verb_not_the_target_message(self) -> None:
    # Source crush 6 and bludgeon 7 are distinct verbs that _object_values()
    # collapses onto target messages 6 and 5. Inference must see the source.
    crush = self._weapon("thing nameless", "a nameless thing", 0, "0 1 6 6")
    bludgeon = self._weapon("thing nameless", "a nameless thing", 0, "0 1 6 7")
    self.assertEqual("WEAPON_TYPE_HEAVY_MACE", mapping.infer_weapon_type(crush).name)
    self.assertEqual("WEAPON_TYPE_CLUB", mapping.infer_weapon_type(bludgeon).name)

  def test_unmapped_source_verbs_still_resolve(self) -> None:
    record = self._weapon("thing nameless", "a nameless thing", 0, "0 1 6 47")
    inference = mapping.infer_weapon_type(record)
    self.assertEqual("WEAPON_TYPE_CLUB", inference.name)
    self.assertEqual("fallback", inference.tier)

  def test_emitted_object_carries_the_inferred_weapon_type(self) -> None:
    record = self._weapon("longsword keen", "a keen longsword", 0, "0 1 8 3")
    emitted = emit_object(record, 2_000_900, _resolver)
    values = emitted.text.splitlines()[6].split()
    self.assertEqual(
        str(mapping.WEAPON_TYPE_IDS["WEAPON_TYPE_LONG_SWORD"]), values[0]
    )
    # The damage-message remap still runs, and the dice are untouched here.
    self.assertEqual(["1", "8", "3"], values[1:4])
    self.assertIn("inferred weapon type", " ".join(emitted.diagnostics))

  def test_non_weapon_objects_keep_their_source_value_zero(self) -> None:
    record = self._source_record(
        b"#901\nlamp~\na brass lamp~\nA brass lamp is here.~\n~\n"
        b"1 0 0\n7 0 0 0\n10 100 0\n"
    )
    emitted = emit_object(record, 2_000_901, _resolver)
    values = emitted.text.splitlines()[6].split()
    self.assertEqual("7", values[0])
    self.assertNotIn("inferred weapon type", " ".join(emitted.diagnostics))


if __name__ == "__main__":
  unittest.main()
