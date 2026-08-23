from __future__ import annotations

from pathlib import Path
import json
import re
import tempfile
import unittest

from wtool_lib.constants import default_repo_root, load_manifest
from wtool_lib.objects import parse_object_file
from wtool_lib.rol_source import parse_active_rol_corpus, parse_rol_source_file
from wtool_lib.rol_transform import EQUIPMENT_POSITION_MAP, emit_object
from wtool_lib.rol_weapon_table import weapon_table
from wtool_lib import rol_weapon_mapping as mapping


# WEAPON_FLAG_RANGED, src/structs.h.
_WEAPON_FLAG_RANGED = 1 << 3

# WEAPON_FLAG_THROWN, src/structs.h.
_WEAPON_FLAG_THROWN = 1 << 4

# The has_ammo_in_pouch() pairings, restated for the kit simulation.
_AMMO_PAIRS = {
    "AMMO_TYPE_ARROW": ("BOW",),
    "AMMO_TYPE_BOLT": ("CROSSBOW", "XBOW"),
    "AMMO_TYPE_STONE": ("SLING",),
    "AMMO_TYPE_DART": ("DART",),
}


def _pairs(ammo_name: str, weapon_name: str) -> bool:
  return any(token in weapon_name for token in _AMMO_PAIRS[ammo_name])


def _resolver(kind: str, vnum: int) -> int:
  return vnum + 2_000_000


class RolWeaponMappingTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.root = default_repo_root()
    cls.source_root = cls.root / "EXAMPLE/RealmsOfLuminari"
    cls.corpus = None
    cls.weapons = []
    cls.launchers = []
    cls.ammunition = []
    if (cls.source_root / "areas").is_dir():
      cls.corpus = parse_active_rol_corpus(cls.source_root, cls.root)
      objects = [record for record in cls.corpus.records if record.kind == "obj"]
      cls.weapons = [
          record
          for record in objects
          if record.values.get("item_type") == mapping.SOURCE_ITEM_TYPE_WEAPON
      ]
      cls.launchers = [
          record
          for record in objects
          if record.values.get("item_type") == mapping.SOURCE_ITEM_TYPE_FIREWEAPON
      ]
      cls.ammunition = [
          record
          for record in objects
          if record.values.get("item_type") == mapping.SOURCE_ITEM_TYPE_MISSILE
      ]

  def _require_reference_corpus(self) -> None:
    if self.corpus is None:
      self.skipTest("ignored RoL reference corpus is not installed")

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
    self._require_reference_corpus()
    self.assertEqual(1319, len(self.weapons))
    for record in self.weapons:
      inference = mapping.infer_weapon_type(record)
      self.assertGreaterEqual(inference.weapon_type, 1, record.record_id)
      self.assertLess(inference.weapon_type, mapping.NUM_WEAPON_TYPES, record.record_id)
      self.assertNotIn(inference.name, mapping.RANGED_WEAPON_TYPES, record.record_id)
      self.assertIn(inference.tier, {"override", "keyword", "fallback"})

  def test_keyword_and_override_tiers_carry_the_corpus(self) -> None:
    self._require_reference_corpus()
    report = mapping.audit(self.corpus.records)
    self.assertEqual(0, report["undefined"])
    self.assertEqual(1319, report["weapons"])
    # The fallback matrix is a safety net, not a workhorse. Every record it
    # still catches is a builder placeholder whose verb and handedness are the
    # only identity it was ever given.
    self.assertLessEqual(report["tiers"]["fallback"], 30)
    self.assertGreaterEqual(report["tiers"]["keyword"], 1200)

  def test_overrides_are_curated_against_records_that_exist(self) -> None:
    self._require_reference_corpus()
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

  def test_javelin_identity_is_preserved_across_source_record_families(self) -> None:
    melee = self._weapon("javelin", "a balanced javelin", 0, "0 1 6 11")
    launcher = self._source_record(
        b"#901\njavelin~\na balanced javelin~\nA balanced javelin lies here.~\n~\n"
        b"6 0 0\n0 1 6 11 0 1 10 14\n1 20 0\n"
    )
    missile = self._source_record(
        b"#902\njavelin~\na balanced javelin~\nA balanced javelin lies here.~\n~\n"
        b"7 0 0\n1 6 10 14\n1 20 0\n"
    )

    self.assertEqual("WEAPON_TYPE_JAVELIN", mapping.infer_weapon_type(melee).name)
    self.assertEqual("WEAPON_TYPE_JAVELIN", mapping.infer_ranged_weapon_type(launcher).name)
    self.assertEqual("WEAPON_TYPE_JAVELIN", mapping.infer_ammunition(missile).name)

    for record in (melee, launcher, missile):
      with self.subTest(source_item_type=record.values["item_type"]):
        _emitted, item_type, values, _economy, _blocks = self._emit(record)
        self.assertEqual(mapping.TARGET_ITEM_WEAPON, item_type)
        self.assertEqual(mapping.WEAPON_TYPE_IDS["WEAPON_TYPE_JAVELIN"], values[0])

  def test_native_javelin_profile_is_thrown_but_not_an_ammo_launcher(self) -> None:
    entry = weapon_table()[mapping.WEAPON_TYPE_IDS["WEAPON_TYPE_JAVELIN"]]
    self.assertTrue(entry.weapon_flags & _WEAPON_FLAG_THROWN)
    self.assertFalse(entry.weapon_flags & _WEAPON_FLAG_RANGED)
    self.assertEqual((1, 6, 2, 30), (entry.num_dice, entry.dice_size, entry.weight, entry.range))

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


  # ------------------------------------------------------------------
  # Ranged weapons, ammunition, and the delivery chain.
  # ------------------------------------------------------------------

  def _emit(self, record):
    emitted = emit_object(record, 2_000_000 + record.vnum, _resolver)
    lines = emitted.text.splitlines()
    # The three numeric rows follow the four tilde-terminated strings, the
    # last of which may span lines.
    tildes = 0
    index = 1
    while tildes < 4:
      if lines[index].endswith("~"):
        tildes += 1
      index += 1
    header = lines[index].split()
    values = [int(value) for value in lines[index + 1].split()]
    economy = [int(value) for value in lines[index + 2].split()]
    blocks = {}
    for offset, line in enumerate(lines[index + 3:]):
      if line in {"G", "H", "I"}:
        blocks[line] = int(lines[index + 3 + offset + 1])
    return emitted, int(header[0]), values, economy, blocks

  def test_ammo_type_table_matches_the_target_header(self) -> None:
    header = (self.root / "src/structs.h").read_text(encoding="utf-8", errors="ignore")
    declared = {
        int(value): name
        for name, value in re.findall(r"#define (AMMO_TYPE_\w+) (\d+)", header)
    }
    self.assertEqual(declared, mapping.AMMO_TYPE_NAMES)
    count = re.search(r"#define NUM_AMMO_TYPES (\d+)", header)
    self.assertIsNotNone(count)
    self.assertEqual(int(count.group(1)), mapping.NUM_AMMO_TYPES)

  def test_ammo_paired_weapon_types_match_has_ammo_in_pouch(self) -> None:
    source = (self.root / "src/combat/assign_wpn_armor.c").read_text(
        encoding="utf-8", errors="ignore"
    )
    body = source[source.index("bool has_ammo_in_pouch("):]
    body = body[: body.index("\n}\n")]
    declared = set(re.findall(r"case (WEAPON_TYPE_\w+):", body))
    self.assertEqual(declared, set(mapping.AMMO_PAIRED_WEAPON_TYPES))
    # The current ranged engine requires an ammo pouch for every ranged type.
    self.assertEqual(mapping.AMMO_PAIRED_WEAPON_TYPES, mapping.RANGED_WEAPON_TYPES)

  def test_weapon_table_covers_every_declared_weapon_type(self) -> None:
    table = weapon_table()
    self.assertEqual(
        {value for value in mapping.WEAPON_TYPE_NAMES if value},
        set(table),
    )
    for weapon_type, entry in table.items():
      self.assertGreaterEqual(entry.num_dice, 1, mapping.WEAPON_TYPE_NAMES[weapon_type])
      self.assertGreaterEqual(entry.dice_size, 1, mapping.WEAPON_TYPE_NAMES[weapon_type])

  def test_every_source_launcher_resolves_to_a_usable_weapon_type(self) -> None:
    self._require_reference_corpus()
    self.assertEqual(51, len(self.launchers))
    for record in self.launchers:
      inference = mapping.infer_ranged_weapon_type(record)
      self.assertGreaterEqual(inference.weapon_type, 1, record.record_id)
      self.assertLess(inference.weapon_type, mapping.NUM_WEAPON_TYPES, record.record_id)
      if inference.name in mapping.RANGED_WEAPON_TYPES:
        self.assertIn(inference.name, mapping.AMMO_PAIRED_WEAPON_TYPES, record.record_id)

  def test_every_source_missile_resolves_to_ammunition_or_a_weapon(self) -> None:
    self._require_reference_corpus()
    self.assertEqual(48, len(self.ammunition))
    retyped = 0
    for record in self.ammunition:
      inference = mapping.infer_ammunition(record)
      if inference.item_type == mapping.TARGET_ITEM_MISSILE:
        self.assertGreaterEqual(inference.ammo_type, 1, record.record_id)
        self.assertLess(inference.ammo_type, mapping.NUM_AMMO_TYPES, record.record_id)
      else:
        retyped += 1
        self.assertNotIn(inference.name, mapping.RANGED_WEAPON_TYPES, record.record_id)
    # The target has no throwing command, so thrown source ammunition becomes
    # the melee weapon it is instead of unusable ammo.
    self.assertEqual(4, retyped)

  def test_declared_range_type_outranks_the_record_name(self) -> None:
    # 'a blackwood shortbow' is declared Long Bow. The declared type is what
    # the source engine used for range, rate of fire, and quiver
    # compatibility, so it wins and the disagreement is reported instead.
    self._require_reference_corpus()
    report = mapping.audit(self.corpus.records)
    disagreements = {row["vnum"]: row for row in report["name_disagreements"]}
    self.assertIn(21721, disagreements)
    self.assertEqual("WEAPON_TYPE_LONG_BOW", disagreements[21721]["resolved"])
    self.assertEqual("WEAPON_TYPE_SHORT_BOW", disagreements[21721]["named"])

  def test_audit_covers_all_three_source_families(self) -> None:
    self._require_reference_corpus()
    report = mapping.audit(self.corpus.records)
    self.assertEqual(1319, report["weapons"])
    self.assertEqual(51, report["ranged_weapons"])
    self.assertEqual(48, report["ammunition"])
    self.assertEqual(4, report["retyped_ammunition"])
    self.assertEqual(0, report["undefined"])
    self.assertEqual(0, report["undefined_ammo"])

  def test_ranged_overrides_are_curated_against_records_that_exist(self) -> None:
    self._require_reference_corpus()
    launchers = {record.vnum for record in self.launchers}
    ammunition = {record.vnum for record in self.ammunition}
    for vnum, entry in mapping.load_ranged_overrides().items():
      self.assertIn(vnum, launchers, f"ranged override {vnum} names no source launcher")
      self.assertTrue(entry["rationale"], f"ranged override {vnum} carries no rationale")
    for vnum, entry in mapping.load_ammunition_overrides().items():
      self.assertIn(vnum, ammunition, f"ammo override {vnum} names no source missile")
      self.assertTrue(entry["rationale"], f"ammo override {vnum} carries no rationale")

  def test_ranged_override_catalog_accepts_native_thrown_weapon_types(self) -> None:
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    path = Path(temporary.name) / "overrides.json"

    path.write_text(
        json.dumps({"ranged_overrides": {"1": {"weapon_type": "WEAPON_TYPE_JAVELIN"}}})
    )
    catalog = mapping.load_catalog(path)
    self.assertEqual(
        "WEAPON_TYPE_JAVELIN", catalog["ranged_overrides"][1]["name"]
    )

  def test_ammunition_override_catalog_rejects_invalid_targets(self) -> None:
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    path = Path(temporary.name) / "overrides.json"

    path.write_text(
        json.dumps({"ammunition_overrides": {"1": {"target": "AMMO_TYPE_UNDEFINED"}}})
    )
    with self.assertRaisesRegex(ValueError, "AMMO_TYPE_UNDEFINED"):
      mapping.load_catalog(path)

    path.write_text(
        json.dumps({"ammunition_overrides": {"1": {"target": "WEAPON_TYPE_LONG_BOW"}}})
    )
    with self.assertRaisesRegex(ValueError, "ranged weapon type"):
      mapping.load_catalog(path)

  def test_emitted_launcher_matches_set_weapon_object(self) -> None:
    self._require_reference_corpus()
    table = weapon_table()
    for record in self.launchers:
      with self.subTest(vnum=record.vnum):
        _emitted, item_type, values, economy, blocks = self._emit(record)
        entry = table[values[0]]
        self.assertEqual(mapping.TARGET_ITEM_WEAPON, item_type)
        self.assertEqual([entry.num_dice, entry.dice_size], values[1:3])
        self.assertEqual(entry.weight, economy[0])
        self.assertEqual(entry.cost + 1, economy[1])
        self.assertEqual(entry.material, blocks["H"])
        self.assertEqual(entry.size, blocks["I"])
        # The source rate of fire must never survive into the target's
        # loaded-ammo counter.
        self.assertEqual(0, values[5])
        self.assertLessEqual(values[4], 10)

  def test_emitted_ammunition_zeroes_the_imbued_spell_and_inverts_durability(self) -> None:
    self._require_reference_corpus()
    for record in self.ammunition:
      with self.subTest(vnum=record.vnum):
        _emitted, item_type, values, _economy, _blocks = self._emit(record)
        if item_type != mapping.TARGET_ITEM_MISSILE:
          continue
        source_values = (list(record.values.get("values", [])) + [0] * 4)[:4]
        self.assertIn(values[0], range(1, mapping.NUM_AMMO_TYPES))
        self.assertEqual(0, values[1])
        self.assertEqual(
            mapping.missile_break_probability(source_values[2]), values[2]
        )
        self.assertEqual(0, values[3])
        self.assertLessEqual(values[4], 10)

  def test_retyped_thrown_ammunition_drops_the_source_missile_type(self) -> None:
    # value[3] on a source missile is its missile type; on a target weapon it
    # indexes attack_hit_text[]. Carrying it across names an unrelated verb.
    record = self._source_record(
        b"#906\ndagger throwing~\na throwing dagger~\nA throwing dagger lies here.~\n~\n"
        b"7 0 0\n2 5 6 9\n1 20 0\n"
    )
    _emitted, item_type, values, _economy, _blocks = self._emit(record)
    self.assertEqual(mapping.TARGET_ITEM_WEAPON, item_type)
    self.assertEqual(mapping.WEAPON_TYPE_IDS["WEAPON_TYPE_DAGGER"], values[0])
    self.assertEqual(0, values[3])

  def test_every_retyped_source_missile_drops_the_missile_type(self) -> None:
    self._require_reference_corpus()
    retyped = 0
    for record in self.ammunition:
      _emitted, item_type, values, _economy, _blocks = self._emit(record)
      if item_type != mapping.TARGET_ITEM_WEAPON:
        continue
      retyped += 1
      self.assertEqual(0, values[3], record.record_id)
    self.assertEqual(4, retyped)

  def test_source_quivers_split_by_the_kind_they_declare(self) -> None:
    self._require_reference_corpus()
    kinds = {}
    for record in self.corpus.records:
      if record.kind != "obj":
        continue
      if record.values.get("item_type") != mapping.SOURCE_ITEM_TYPE_QUIVER:
        continue
      values = (list(record.values.get("values", [])) + [0] * 4)[:4]
      _emitted, item_type, emitted_values, _economy, _blocks = self._emit(record)
      kinds[item_type] = kinds.get(item_type, 0) + 1
      expected = (
          mapping.TARGET_ITEM_CONTAINER
          if values[3] == mapping.SOURCE_QUIVER_THROWING
          else mapping.TARGET_ITEM_AMMO_POUCH
      )
      self.assertEqual(expected, item_type, record.record_id)
      # The target reads value[3] as the corpse flag.
      self.assertEqual(0, emitted_values[3], record.record_id)
    self.assertEqual(
        {mapping.TARGET_ITEM_AMMO_POUCH: 24, mapping.TARGET_ITEM_CONTAINER: 20}, kinds
    )

  def test_hitroll_and_damroll_become_the_enhancement_bonus(self) -> None:
    record = self._source_record(
        b"#902\nbow long~\na long bow~\nA long bow lies here.~\n~\n"
        b"6 0 0\n0 1 6 3 2 5 10 2\n10 100 0\n"
        b"A\n18 5\nA\n19 3\n"
    )
    emitted, item_type, values, _economy, _blocks = self._emit(record)
    self.assertEqual(mapping.TARGET_ITEM_WEAPON, item_type)
    self.assertEqual(
        mapping.WEAPON_TYPE_IDS["WEAPON_TYPE_LONG_BOW"], values[0]
    )
    self.assertEqual(4, values[4])
    self.assertNotIn("\nA\n", emitted.text)
    self.assertIn("restated source hitroll 5 and damroll 3", " ".join(emitted.diagnostics))

  def test_enhancement_bonus_clamps_and_reports(self) -> None:
    record = self._source_record(
        b"#903\nsword~\na sword~\nA sword lies here.~\n~\n"
        b"5 0 0\n0 1 6 3\n10 100 0\n"
        b"A\n18 100\nA\n19 100\n"
    )
    emitted, _item_type, values, _economy, _blocks = self._emit(record)
    self.assertEqual(10, values[4])
    self.assertIn("clamped enhancement bonus 100 to 10", " ".join(emitted.diagnostics))

    cursed = self._source_record(
        b"#904\nsword~\na sword~\nA sword lies here.~\n~\n"
        b"5 0 0\n0 1 6 3\n10 100 0\n"
        b"A\n18 -3\nA\n19 -3\n"
    )
    emitted, _item_type, values, _economy, _blocks = self._emit(cursed)
    self.assertEqual(0, values[4])
    self.assertIn("clamped enhancement bonus -3 to 0", " ".join(emitted.diagnostics))

  def test_converted_weapon_carries_only_take_and_wield(self) -> None:
    # Source wear bit 15 is WIELD and bit 1 is FINGER; a weapon that also wore
    # in another slot loses that, exactly as set_weapon_object() does.
    record = self._source_record(
        b"#905\nring sword~\na sword ring~\nA sword ring lies here.~\n~\n"
        b"5 0 32770\n0 1 6 3\n10 100 0\n"
    )
    emitted, _item_type, _values, _economy, blocks = self._emit(record)
    header = [line for line in emitted.text.splitlines() if line.startswith("5 ")][0]
    # Four flag words each for extra, wear, affects, and affects2; "an" is
    # ITEM_WEAR_TAKE plus ITEM_WEAR_WIELD.
    self.assertEqual(["an", "0", "0", "0"], header.split()[5:9])
    self.assertIn("cleared object wear flags", " ".join(emitted.diagnostics))
    self.assertIn("G", blocks)

  def test_every_emitted_ranged_record_survives_the_target_parser(self) -> None:
    self._require_reference_corpus()
    manifest = load_manifest(self.root / "scripts/world/wtool_constants.json")
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    text = "".join(
        emit_object(record, 2_000_000 + record.vnum, _resolver).text
        for record in self.launchers + self.ammunition
    )
    path = Path(temporary.name) / "20999.obj"
    path.write_text(text + "$~\n", encoding="ascii", newline="\n")
    result = parse_object_file(path, "obj/20999.obj", manifest, set())
    self.assertTrue(result.complete)
    self.assertEqual(99, len(result.records))
    self.assertEqual(
        [], [finding for finding in result.findings if finding.severity == "error"]
    )

  def test_every_converted_kit_would_pass_has_ammo_in_pouch(self) -> None:
    """Walk each zone's M/E/P chain and check the kits it actually builds."""

    self._require_reference_corpus()
    table = weapon_table()
    emitted: dict[int, tuple[int, list[int]]] = {}
    for record in self.corpus.records:
      if record.kind != "obj":
        continue
      if record.values.get("item_type") not in {
          mapping.SOURCE_ITEM_TYPE_WEAPON,
          mapping.SOURCE_ITEM_TYPE_FIREWEAPON,
          mapping.SOURCE_ITEM_TYPE_MISSILE,
          mapping.SOURCE_ITEM_TYPE_QUIVER,
      }:
        continue
      _result, item_type, values, _economy, _blocks = self._emit(record)
      emitted[record.vnum] = (item_type, values)

    ammo_for = {
        mapping.AMMO_TYPE_IDS[name]: {
            mapping.WEAPON_TYPE_IDS[weapon]
            for weapon in mapping.AMMO_PAIRED_WEAPON_TYPES
            if _pairs(name, weapon)
        }
        for name in ("AMMO_TYPE_ARROW", "AMMO_TYPE_BOLT", "AMMO_TYPE_STONE", "AMMO_TYPE_DART")
    }
    pouch_position = EQUIPMENT_POSITION_MAP[23]
    kits = 0
    for record in self.corpus.records:
      if record.kind != "zon":
        continue
      launcher = None
      pouches: list[int] = []
      containers: dict[int, list[int]] = {}
      for directive in record.directives:
        token = directive["token"]
        arguments = [int(value) for value in directive.get("arguments", [])]
        if token == "M":
          launcher, pouches, containers = None, [], {}
        elif token == "E" and len(arguments) >= 4:
          prototype, position = arguments[1], arguments[3]
          entry = emitted.get(prototype)
          if entry is None:
            continue
          item_type, values = entry
          if item_type == mapping.TARGET_ITEM_WEAPON and table[
              values[0]
          ].weapon_flags & _WEAPON_FLAG_RANGED:
            launcher = values[0]
          if (
              EQUIPMENT_POSITION_MAP.get(position) == pouch_position
              and item_type == mapping.TARGET_ITEM_AMMO_POUCH
          ):
            pouches.append(prototype)
        elif token == "P" and len(arguments) >= 4:
          containers.setdefault(arguments[3], []).append(arguments[1])
        if launcher is None:
          continue
        for pouch in pouches:
          for content in containers.get(pouch, ()):
            entry = emitted.get(content)
            if entry is None or entry[0] != mapping.TARGET_ITEM_MISSILE:
              continue
            kits += 1
            self.assertIn(
                launcher,
                ammo_for[entry[1][0]],
                f"zone {record.vnum} loads ammo {content} that its launcher cannot fire",
            )
    self.assertGreater(kits, 0)

if __name__ == "__main__":
  unittest.main()
