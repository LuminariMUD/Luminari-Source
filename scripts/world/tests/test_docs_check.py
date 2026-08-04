from __future__ import annotations

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


if __name__ == "__main__":
  unittest.main()
