from __future__ import annotations

import json
from pathlib import Path
import tempfile
import unittest

from wtool_lib.constants import (
    ExtractionError,
    _filter_luminari_branch,
    check_manifest,
    default_manifest_path,
    default_repo_root,
    extract_manifest,
    manifest_text,
)
from wtool_lib.flags import decode_tokens, encode_bits, resolve_names, resolve_set
from wtool_lib.spec_registry import SpecRegistryError, extract_spec_names


class ConstantsTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.repo_root = default_repo_root()
    cls.manifest = extract_manifest(cls.repo_root)

  def test_room_table_matches_source_contract(self) -> None:
    entries = self.manifest["tables"]["room"]["entries"]
    self.assertEqual(47, len(entries))
    self.assertEqual("ROOM_DARK", entries[0]["macro"])
    self.assertEqual("Dark", entries[0]["name"])
    self.assertEqual("ROOM_DOCKABLE", entries[41]["macro"])
    self.assertEqual("Dockable", entries[41]["name"])
    self.assertEqual("ROOM_PSP_REGEN", entries[45]["macro"])
    self.assertEqual("Psionic-Regeneration", entries[45]["name"])
    self.assertEqual("ROOM_ROL_HOME_RESET", entries[46]["macro"])
    self.assertEqual("RoL-Home-Reset", entries[46]["name"])

  def test_quest_tables_match_source_contract(self) -> None:
    quest_types = self.manifest["tables"]["quest-types"]
    self.assertEqual("src/quest/quest.c:quest_types", quest_types["source_table"])
    self.assertEqual(25, len(quest_types["entries"]))
    self.assertEqual("AQ_OBJ_FIND", quest_types["entries"][0]["macro"])
    self.assertEqual("Acquire Object", quest_types["entries"][0]["name"])
    self.assertEqual("AQ_DIALOGUE", quest_types["entries"][24]["macro"])
    self.assertEqual("Dialogue Quest", quest_types["entries"][24]["name"])

    quest_flags = self.manifest["tables"]["quest"]
    self.assertEqual(1, quest_flags["serialized_chunks"])
    self.assertEqual(
        ["AQ_REPEATABLE", "AQ_REPLACE_OBJ_REWARD"],
        [entry["macro"] for entry in quest_flags["entries"]],
    )
    self.assertEqual(
        ["REPEATABLE", "REPLACE-OBJ-REWARD"],
        [entry["name"] for entry in quest_flags["entries"]],
    )

  def test_hlquest_enums_and_command_codes_match_source_contract(self) -> None:
    entry_types = self.manifest["tables"]["hlquest-entry-types"]
    self.assertEqual(
        ["QUEST_ASK", "QUEST_GIVE", "QUEST_ROOM"],
        [entry["macro"] for entry in entry_types["entries"]],
    )
    commands = self.manifest["tables"]["hlquest-commands"]
    self.assertEqual("CIOMADTXFKUSPE", "".join(entry["code"] for entry in commands["entries"]))
    self.assertEqual(14, len(commands["entries"]))
    self.assertEqual("QUEST_COMMAND_COINS", commands["entries"][0]["macro"])
    self.assertEqual("QUEST_COMMAND_EXPERIENCE", commands["entries"][-1]["macro"])

  def test_mission_difficulties_and_quest_limits_match_source(self) -> None:
    difficulties = self.manifest["tables"]["mission-difficulties"]["entries"]
    self.assertEqual(
        ["easy", "normal", "tough", "challenging", "arduous", "severe"],
        [entry["name"] for entry in difficulties],
    )
    expected_limits = {
        "AQ_UNDEFINED": -1,
        "MAX_GOLD": 2140000000,
        "MAX_QUEST_DESC": 75,
        "MAX_QUEST_MSG": 4096,
        "MAX_QUEST_NAME": 40,
        "NUM_AQ_FLAGS": 2,
        "NUM_AQ_TYPES": 25,
        "NUM_CHURCHES": 13,
        "NUM_MISSION_DIFFICULTIES": 6,
        "RACE_LICH": 45,
        "RACE_UNDEFINED": -1,
        "RACE_VAMPIRE": 46,
        "SPELL_RESERVED_DBC": 0,
        "TOP_SKILL_DEFINE": 3500,
    }
    self.assertEqual(
        expected_limits,
        {symbol: self.manifest["limits"][symbol]["value"] for symbol in expected_limits},
    )

  def test_bounded_blocks_keep_aliases_without_prefix_collisions(self) -> None:
    zone = self.manifest["tables"]["zone"]["entries"]
    positions = self.manifest["tables"]["positions"]["entries"]
    mob_macros = {
        entry["macro"] for entry in self.manifest["tables"]["mob"]["entries"]
    }
    self.assertIn("ZONE_OPEN", zone[3]["aliases"])
    self.assertIn("POS_CRAWLING", positions[5]["aliases"])
    self.assertNotIn("MOB_DIRE_SPIDER", mob_macros)

  def test_rol_totem_spirit_flag_matches_runtime_contract(self) -> None:
    entries = self.manifest["tables"]["mob"]["entries"]
    self.assertEqual(124, len(entries))
    self.assertEqual("MOB_ROL_TOTEM_SPIRIT", entries[123]["macro"])
    self.assertEqual("RoL-Totem-Spirit", entries[123]["name"])

  def test_luminari_campaign_filter_selects_non_campaign_branch(self) -> None:
    source = "#ifdef CAMPAIGN_FR\nfr\n#else\nluminari\n#endif\n"
    self.assertEqual("luminari\n", _filter_luminari_branch(source))

  def test_campaign_filter_restores_nested_parent_state(self) -> None:
    source = (
        "#ifndef CAMPAIGN_FR\n"
        "outer\n"
        "#ifdef CAMPAIGN_DL\n"
        "dragonlance\n"
        "#else\n"
        "nested-luminari\n"
        "#endif\n"
        "after-nested\n"
        "#endif\n"
    )
    self.assertEqual("outer\nnested-luminari\nafter-nested\n", _filter_luminari_branch(source))

  def test_campaign_filter_fails_closed_for_unknown_conditions(self) -> None:
    with self.assertRaises(ExtractionError):
      _filter_luminari_branch("#ifdef CAMPAIGN_UNKNOWN\nvalue\n#endif\n")

  def test_checked_in_manifest_is_current(self) -> None:
    current, message = check_manifest(self.repo_root)
    self.assertTrue(current, message)
    checked_in = default_manifest_path(self.repo_root).read_text(encoding="ascii")
    self.assertEqual(manifest_text(self.manifest), checked_in)

  def test_check_detects_deliberately_stale_copy(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      stale = Path(directory) / "constants.json"
      data = json.loads(manifest_text(self.manifest))
      data["flag_encoding"]["chunk_width"] = 31
      stale.write_text(json.dumps(data, sort_keys=True), encoding="ascii")
      current, _ = check_manifest(self.repo_root, stale)
      self.assertFalse(current)

  def test_flag_above_31_round_trips_in_later_chunk(self) -> None:
    tokens = encode_bits({0, 41, 95, 126})
    self.assertEqual(("a", "j", "F", "E"), tokens)
    decoded = decode_tokens(list(tokens), entry_count=127)
    self.assertEqual({0, 41, 95, 126}, set(decoded.bits))
    self.assertEqual((), decoded.issues)

  def test_quest_flags_round_trip_as_exactly_one_chunk(self) -> None:
    _, table = resolve_set(self.manifest, "quest")
    chunks = table["serialized_chunks"]
    tokens = encode_bits({0, 1}, serialized_chunks=chunks)
    self.assertEqual(("ab",), tokens)
    decoded = decode_tokens(list(tokens), entry_count=2, serialized_chunks=chunks)
    self.assertEqual({0, 1}, set(decoded.bits))
    self.assertEqual((3,), decoded.chunks)
    self.assertEqual((), decoded.issues)

  def test_quest_flags_reject_a_second_serialized_chunk(self) -> None:
    decoded = decode_tokens(["a", "b"], entry_count=2, serialized_chunks=1)
    self.assertEqual({0}, set(decoded.bits))
    self.assertEqual({"FLG003"}, {issue.code for issue in decoded.issues})

  def test_existing_flag_sets_still_emit_four_chunks(self) -> None:
    _, room = resolve_set(self.manifest, "room")
    self.assertEqual(4, room["serialized_chunks"])
    self.assertEqual(("a", "0", "0", "0"), encode_bits({0}))

  def test_high_local_decoder_bit_is_diagnosed(self) -> None:
    decoded = decode_tokens(["G"], entry_count=127)
    self.assertEqual("FLG002", decoded.issues[0].code)

  def test_all_invalid_flag_encoding_classes_are_diagnosed(self) -> None:
    decoded = decode_tokens(["?", "-1", "a", "0"], entry_count=64)
    self.assertEqual(
        {"FLG001", "FLG004", "FLG006"},
        {issue.code for issue in decoded.issues},
    )

  def test_reverse_directions_match_runtime_table(self) -> None:
    directions = self.manifest["tables"]["directions"]["entries"]
    self.assertEqual([2, 3, 0, 1, 5, 4, 8, 9, 6, 7], [
        entry["reverse_index"] for entry in directions
    ])

  def test_numeric_masks_are_valid(self) -> None:
    decoded = decode_tokens(["12", "0", "0", "0"], entry_count=42)
    self.assertEqual({2, 3}, set(decoded.bits))
    self.assertEqual((), decoded.issues)

  def test_names_accept_display_macro_and_alias_forms(self) -> None:
    _, table = resolve_set(self.manifest, "zone")
    self.assertEqual({3}, resolve_names(table, ["Open for Players"]))
    self.assertEqual({3}, resolve_names(table, ["ZONE_OPEN"]))


class SpecRegistryTests(unittest.TestCase):
  def test_current_registry_exposes_canonical_and_alias_names(self) -> None:
    names = extract_spec_names(default_repo_root())
    self.assertEqual(77, len(names))
    self.assertIn("bank", names)
    self.assertIn("guild", names)
    self.assertIn("guildmaster", names)
    self.assertIn("greyhawk ship commands", names)
    self.assertIn("rol guild room", names)
    self.assertIn("rol corpse devourer", names)
    self.assertIn("rol poison bite", names)
    self.assertIn("rol thief", names)
    self.assertIn("rol magic pool", names)
    self.assertIn("rol auto distributor", names)
    self.assertIn("rol shadow giant", names)
    self.assertIn("rol major beholder", names)
    self.assertIn("rol lich energy drain", names)
    self.assertIn("rol trade bandit", names)
    self.assertIn("rol shaman totem", names)
    self.assertIn("rol ship", names)
    self.assertIn("rol ship control", names)
    self.assertIn("rol ship exit", names)
    self.assertIn("rol ship lookout", names)
    self.assertIn("rol ship navigator", names)

  def test_registry_extractor_requires_referenced_alias_initializers(self) -> None:
    source = """\
static const struct spec_definition spec_definitions[] = {
    {
        .canonical_name = "Canonical",
        .aliases = missing_aliases,
    },
};
"""
    with tempfile.TemporaryDirectory() as directory:
      registry = Path(directory) / "src/spec/spec_registry.c"
      registry.parent.mkdir(parents=True)
      registry.write_text(source, encoding="ascii")
      with self.assertRaisesRegex(SpecRegistryError, "missing_aliases"):
        extract_spec_names(Path(directory))

  def test_registry_extractor_ignores_commented_entries(self) -> None:
    source = """\
static const char *const live_aliases[] = {
    "Live Alias", /* "Commented Alias" */
};
static const struct spec_definition spec_definitions[] = {
    {
        .canonical_name = "Live",
        // .canonical_name = "Commented Canonical",
        .aliases = live_aliases,
        /* .aliases = missing_aliases, } */
    },
};
"""
    with tempfile.TemporaryDirectory() as directory:
      registry = Path(directory) / "src/spec/spec_registry.c"
      registry.parent.mkdir(parents=True)
      registry.write_text(source, encoding="ascii")
      self.assertEqual({"live", "live alias"}, extract_spec_names(Path(directory)))


if __name__ == "__main__":
  unittest.main()
