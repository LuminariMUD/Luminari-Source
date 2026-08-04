from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from wtool_lib.constants import default_repo_root, load_manifest
from wtool_lib.hlquests import parse_hlquest_file


def ask_entry(keywords: str = "question", reply: str = "answer", marker: str = "A!") -> str:
  return f"{marker}\n{keywords}~\n{reply}~\n"


def give_entry(commands: str = "I C 5 0\nO C 10 0\n", marker: str = "Q!") -> str:
  return f"{marker}\nThe host accepts the offering.~\n{commands}S\n"


def room_entry(room: str = "10000", commands: str = "", marker: str = "R!") -> str:
  return f"{marker}\n{room}\nThe room reacts.~\n{commands}S\n"


class HlQuestContractTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.repo_root = default_repo_root()
    cls.fixtures = cls.repo_root / "scripts/world/tests/fixtures/phase2"
    cls.manifest = load_manifest()

  def read_lines(self, relative_path: str) -> list[str]:
    path = self.fixtures / relative_path
    content = path.read_bytes()
    self.assertNotIn(b"\r", content)
    content.decode("ascii")
    return content.decode("ascii").splitlines()

  def parse_text(self, content: str, name: str = "100.hlq"):
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    path = Path(temporary.name) / name
    path.write_text(content, encoding="ascii")
    return parse_hlquest_file(path, name, self.manifest)

  def test_canonical_writer_contract_covers_all_command_codes(self) -> None:
    lines = self.read_lines("contracts/hlq/canonical.hlq")
    expected_codes = "".join(
        entry["code"]
        for entry in self.manifest["tables"]["hlquest-commands"]["entries"]
    )
    input_codes = "".join(line.split()[1] for line in lines if line.startswith("I "))
    output_codes = "".join(line.split()[1] for line in lines if line.startswith("O "))
    self.assertEqual("CI", input_codes)
    self.assertEqual(expected_codes, output_codes)
    self.assertEqual(["A!", "Q!", "R!"], [line for line in lines if line in {"A!", "Q!", "R!"}])
    self.assertEqual(2, lines.count("S"))
    self.assertEqual("$~", lines[-1])

    path = self.fixtures / "contracts/hlq/canonical.hlq"
    result = parse_hlquest_file(path, "canonical.hlq", self.manifest)
    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    self.assertEqual(1, len(result.records))
    record = result.records[0]
    self.assertEqual(10000, record.host_mobile_vnum)
    self.assertEqual(["A", "Q", "R"], [entry.marker for entry in record.entries])
    self.assertEqual([1, 2, 3], [entry.physical_ordinal for entry in record.entries])
    self.assertEqual(
        [3, 2, 1],
        [entry.effective_runtime_ordinal for entry in record.entries],
    )
    give = record.entries[1]
    self.assertEqual(["C", "I"], [command.code for command in give.input_commands])
    self.assertEqual(
        [2, 1],
        [command.effective_runtime_ordinal for command in give.input_commands],
    )
    self.assertEqual(
        list(range(12)),
        [command.command_type for command in give.output_commands],
    )
    self.assertEqual(
        list(range(1, 13)),
        [command.effective_runtime_ordinal for command in give.output_commands],
    )
    self.assertGreater(give.commands[0].field_spans["value"].column, 1)

  def test_complete_fixture_has_normal_and_mini_hlq_packages(self) -> None:
    complete = self.fixtures / "complete/hlq"
    self.assertEqual("100.hlq\n$\n", (complete / "index").read_text(encoding="ascii"))
    self.assertEqual(
        (complete / "index").read_bytes(),
        (complete / "index.mini").read_bytes(),
    )
    lines = self.read_lines("complete/hlq/100.hlq")
    self.assertEqual("#10000", lines[0])
    self.assertEqual(["A!", "Q!", "R!"], [line for line in lines if line in {"A!", "Q!", "R!"}])

    result = parse_hlquest_file(complete / "100.hlq", "hlq/100.hlq", self.manifest)
    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    self.assertEqual([10000], [record.vnum for record in result.records])
    self.assertEqual(3, len(result.records[0].entries))

  def test_multiple_hosts_multiline_strings_comments_and_blanks(self) -> None:
    content = (
        "* host comment\n\n#10000\nA\nfirst keyword line\nsecond keyword line~\n"
        "first reply line\nsecond reply line~\n#10001\n"
        + give_entry(commands="* command comment\n\nI I 20000 0\n")
        + "$~\n"
    )
    result = self.parse_text(content)
    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    self.assertEqual([10000, 10001], [record.vnum for record in result.records])
    ask = result.records[0].entries[0]
    self.assertFalse(ask.approved)
    self.assertEqual("first keyword line\r\nsecond keyword line", ask.keywords)
    self.assertEqual("first reply line\r\nsecond reply line", ask.reply_message)

  def test_noncanonical_approval_suffix_preserves_loader_approval(self) -> None:
    result = self.parse_text("#10000\n" + ask_entry(marker="A?") + "$\n")
    self.assertTrue(result.complete)
    self.assertEqual(["HLQ008"], [finding.code for finding in result.findings])
    entry = result.records[0].entries[0]
    self.assertTrue(entry.approved)
    self.assertEqual("?", entry.approval_suffix)

  def test_entry_before_host_is_discarded_and_next_host_recovers(self) -> None:
    content = (
        give_entry(commands="I I 20000 0\n")
        + "#10100\n"
        + ask_entry()
        + "$~\n"
    )
    result = self.parse_text(content)
    self.assertFalse(result.complete)
    self.assertEqual([10100], [record.vnum for record in result.records])
    self.assertEqual(["HLQ006"], [finding.code for finding in result.findings])
    self.assertEqual(1, len(result.records[0].entries))

  def test_malformed_host_does_not_reuse_previous_host_or_room_state(self) -> None:
    content = (
        "#not-a-number\n"
        + room_entry("99999")
        + "#10200\n"
        + ask_entry()
        + "$~\n"
    )
    result = self.parse_text(content)
    self.assertFalse(result.complete)
    self.assertEqual([10200], [record.vnum for record in result.records])
    self.assertEqual({"HLQ005", "HLQ006"}, {finding.code for finding in result.findings})

  def test_invalid_room_number_is_not_replaced_with_stale_state(self) -> None:
    content = "#10300\n" + room_entry("invalid") + ask_entry() + "$~\n"
    result = self.parse_text(content)
    self.assertFalse(result.complete)
    self.assertEqual(["R", "A"], [entry.marker for entry in result.records[0].entries])
    self.assertIsNone(result.records[0].entries[0].room_vnum)
    self.assertEqual("question", result.records[0].entries[1].keywords)
    self.assertIn("HLQ009", {finding.code for finding in result.findings})

  def test_missing_room_number_recovers_at_next_credible_entry(self) -> None:
    content = "#10400\nR!\n" + ask_entry() + "$~\n"
    result = self.parse_text(content)
    self.assertFalse(result.complete)
    record = result.records[0]
    self.assertEqual(["R", "A"], [entry.marker for entry in record.entries])
    self.assertIsNone(record.entries[0].room_vnum)
    self.assertEqual("question", record.entries[1].keywords)
    self.assertEqual({"HLQ009"}, {finding.code for finding in result.findings})

  def test_signed_command_values_and_sscanf_trailing_data_are_preserved(self) -> None:
    commands = "I C -5 -1\nO I 20000 0ignored\n"
    result = self.parse_text("#10500\n" + give_entry(commands) + "$~\n")
    self.assertTrue(result.complete)
    self.assertEqual({"HLQ010"}, {finding.code for finding in result.findings})
    parsed = result.records[0].entries[0].commands
    self.assertEqual((-5, -1), (parsed[0].value, parsed[0].location))
    self.assertEqual((20000, 0), (parsed[1].value, parsed[1].location))

  def test_short_and_overflowing_commands_are_incomplete_without_defaults(self) -> None:
    commands = "I C 5\nO I 2147483648 0\nO C 2 0\n"
    result = self.parse_text("#10600\n" + give_entry(commands) + "$~\n")
    self.assertFalse(result.complete)
    parsed = result.records[0].entries[0].commands
    self.assertEqual(3, len(parsed))
    self.assertEqual(5, parsed[0].value)
    self.assertIsNone(parsed[0].location)
    self.assertIsNone(parsed[1].value)
    self.assertEqual((2, 0), (parsed[2].value, parsed[2].location))
    self.assertEqual({"HLQ010"}, {finding.code for finding in result.findings})

  def test_unknown_code_and_invalid_direction_remain_distinct(self) -> None:
    commands = "I Z 1 2\nX C 3 4\nO C 5 6\n"
    result = self.parse_text("#10700\n" + give_entry(commands) + "$~\n")
    self.assertFalse(result.complete)
    entry = result.records[0].entries[0]
    self.assertEqual({"HLQ011", "HLQ012"}, {finding.code for finding in result.findings})
    self.assertIsNone(entry.commands[0].command_type)
    self.assertFalse(entry.commands[1].effective)
    self.assertIsNone(entry.commands[1].effective_runtime_ordinal)
    self.assertEqual(1, entry.commands[2].effective_runtime_ordinal)

  def test_missing_chain_terminator_recovers_at_next_entry(self) -> None:
    content = "#10800\n" + give_entry(commands="I C 1 0\n", marker="Q!")
    content = content.replace("S\n", "", 1) + ask_entry() + "$~\n"
    result = self.parse_text(content)
    self.assertFalse(result.complete)
    record = result.records[0]
    self.assertEqual(["Q", "A"], [entry.marker for entry in record.entries])
    self.assertEqual("question", record.entries[1].keywords)
    self.assertEqual({"HLQ013"}, {finding.code for finding in result.findings})

  def test_command_chain_eof_reports_chain_and_file_incompleteness(self) -> None:
    result = self.parse_text("#10900\nQ!\nreply~\nI C 1 0\n")
    self.assertFalse(result.complete)
    self.assertEqual({"HLQ013", "HLQ014"}, {finding.code for finding in result.findings})
    self.assertFalse(result.records[0].complete)

  def test_unknown_top_level_marker_models_loader_truncation(self) -> None:
    content = "#11000\n" + ask_entry() + "X\n#11001\n" + ask_entry() + "$~\n"
    result = self.parse_text(content)
    self.assertFalse(result.complete)
    self.assertEqual([11000], [record.vnum for record in result.records])
    self.assertEqual({"HLQ007"}, {finding.code for finding in result.findings})

  def test_zero_byte_file_and_content_after_terminator_are_incomplete(self) -> None:
    empty = self.parse_text("", "empty.hlq")
    self.assertFalse(empty.complete)
    self.assertEqual({"HLQ014"}, {finding.code for finding in empty.findings})

    trailing = self.parse_text("$~\n#11100\n" + ask_entry(), "trailing.hlq")
    self.assertFalse(trailing.complete)
    self.assertEqual({"HLQ015"}, {finding.code for finding in trailing.findings})
    self.assertEqual([], trailing.records)

  def test_overlong_get_line_input_is_source_located(self) -> None:
    command = "O C 1 0" + (" " * 250)
    result = self.parse_text("#11200\n" + give_entry(commands=command + "\n") + "$~\n")
    self.assertFalse(result.complete)
    finding = next(finding for finding in result.findings if finding.code == "HLQ001")
    self.assertEqual(4, finding.span.line)


if __name__ == "__main__":
  unittest.main()
