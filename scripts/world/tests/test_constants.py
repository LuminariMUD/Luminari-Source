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


class ConstantsTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.repo_root = default_repo_root()
    cls.manifest = extract_manifest(cls.repo_root)

  def test_room_table_matches_source_contract(self) -> None:
    entries = self.manifest["tables"]["room"]["entries"]
    self.assertEqual(42, len(entries))
    self.assertEqual("ROOM_DARK", entries[0]["macro"])
    self.assertEqual("Dark", entries[0]["name"])
    self.assertEqual("ROOM_DOCKABLE", entries[41]["macro"])
    self.assertEqual("Dockable", entries[41]["name"])

  def test_bounded_blocks_keep_aliases_without_prefix_collisions(self) -> None:
    zone = self.manifest["tables"]["zone"]["entries"]
    positions = self.manifest["tables"]["positions"]["entries"]
    mob_macros = {
        entry["macro"] for entry in self.manifest["tables"]["mob"]["entries"]
    }
    self.assertIn("ZONE_OPEN", zone[3]["aliases"])
    self.assertIn("POS_CRAWLING", positions[5]["aliases"])
    self.assertNotIn("MOB_DIRE_SPIDER", mob_macros)

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


if __name__ == "__main__":
  unittest.main()
