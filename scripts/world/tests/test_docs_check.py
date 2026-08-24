from __future__ import annotations

import re
import unittest

from wtool_lib.constants import default_repo_root, load_manifest
from wtool_lib.docs_check import (
    COMMAND_SECTIONS,
    TABLE_SPECS,
    _command_findings,
    _documented_commands,
    _encoding_findings,
    _table_findings,
    validate_docs,
)
from wtool_lib.rol_transform import _SOURCE_SPELL_MAP


ROL_IMPLEMENTED_GAP_SOURCE_IDS = (
    20, 54, 62, 84, 88, 89, 90, 119, 154, 163, 171, 174, 175, 176, 181,
    182, 228, 230, 231, 232, 233, 235, 237, 297, 298, 299, 301, 302, 303,
    305, 319, 321, 322, 329, 332, 334, 341, 343, 349, 350, 351, 353, 354,
    359, 362, 366, 370, 371, 376, 377, 378, 380, 381, 392, 397, 426, 435,
    437, 438, 442, 445, 447, 450, 452, 453, 459, 467, 470, 471, 473, 475,
    476, 478, 479, 482, 483, 484, 485, 487, 488, 492, 505, 514, 515, 518,
    520, 522, 524, 525,
)


class DocumentationCheckTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.repo_root = default_repo_root()
    cls.manifest = load_manifest(cls.repo_root / "scripts/world/wtool_constants.json")

  def test_repository_world_docs_match_source_contract(self) -> None:
    result = validate_docs(self.repo_root, self.manifest, check_generated=False)
    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)

  def test_table_index_drift_is_reported(self) -> None:
    spec = TABLE_SPECS[0]
    text = (self.repo_root / spec.path).read_text(encoding="ascii")
    text = text.replace("| 41 | ROOM_DOCKABLE", "| 40 | ROOM_DOCKABLE", 1)
    findings = _table_findings(text, spec, self.manifest["tables"][spec.table_key]["entries"])
    self.assertTrue(any(finding.code == "DOC005" for finding in findings))

  def test_quest_system_source_tables_are_registered(self) -> None:
    registered = {(spec.path, spec.table_key) for spec in TABLE_SPECS}
    self.assertTrue(
        {
            ("docs/world_game-data/QUEST_FILE_FORMAT.md", "quest-types"),
            ("docs/world_game-data/QUEST_FILE_FORMAT.md", "quest"),
            ("docs/world_game-data/HLQUEST_FILE_FORMAT.md", "hlquest-entry-types"),
            ("docs/world_game-data/HLQUEST_FILE_FORMAT.md", "hlquest-commands"),
        }.issubset(registered)
    )

  def test_explicit_command_section_extracts_registered_commands(self) -> None:
    spec = COMMAND_SECTIONS[0]
    text = (self.repo_root / spec.path).read_text(encoding="ascii")
    commands, findings = _documented_commands(text, spec)
    self.assertEqual([], findings)
    command_names = {command for command, _ in commands}
    self.assertIn("redit", command_names)
    self.assertIn("hlqedit", command_names)
    self.assertIn("pathlist", command_names)

  def test_unregistered_command_in_reference_is_reported(self) -> None:
    spec = COMMAND_SECTIONS[0]
    path = self.repo_root / spec.path
    original = path.read_text(encoding="ascii")
    mutated = original.replace("| `redit` |", "| `inventedit` |", 1)
    texts = {item.path: (self.repo_root / item.path).read_text(encoding="ascii") for item in COMMAND_SECTIONS}
    texts[spec.path] = mutated
    findings = _command_findings(self.repo_root, texts)
    self.assertTrue(any("inventedit" in finding.message for finding in findings))

  def test_encoding_contract_reports_non_ascii_and_crlf(self) -> None:
    text, findings = _encoding_findings("fixture.md", "bad \N{RIGHTWARDS ARROW}\r\n".encode())
    self.assertIsNotNone(text)
    self.assertEqual({"DOC003", "DOC004"}, {finding.code for finding in findings})

  def test_rol_implemented_gap_spells_have_flat_help_coverage(self) -> None:
    spell_names = [
        _SOURCE_SPELL_MAP[source_id][0]
        for source_id in ROL_IMPLEMENTED_GAP_SOURCE_IDS
    ]

    help_lines = (self.repo_root / "lib/text/help/help.hlp").read_text(
        encoding="utf-8"
    ).splitlines()
    help_keywords: set[str] = set()
    expect_header = True
    for line in help_lines:
      if expect_header:
        if not line or line.startswith("#") or line == "$~":
          continue
        help_keywords.update(token.upper() for token in line.split())
        expect_header = False
      elif line.startswith("#"):
        expect_header = True

    expected_keywords = {
        re.sub(r"[^A-Z0-9]+", "-", name.upper()).strip("-")
        for name in spell_names
    }
    self.assertEqual(89, len(spell_names))
    self.assertEqual(89, len(expected_keywords))
    self.assertEqual(set(), expected_keywords - help_keywords)


if __name__ == "__main__":
  unittest.main()
