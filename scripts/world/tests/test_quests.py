from __future__ import annotations

from pathlib import Path
import json
import tempfile
import unittest

from tests.test_cli import make_world, run_cli, tree_hash
from wtool_lib.constants import default_repo_root, load_manifest
from wtool_lib.quests import parse_quest_file
from wtool_lib.world import load_indexed_world_data, validate_explicit_paths


def quest_record(
    vnum: int,
    flags: str = "0",
    row_two: str = "1 0 1 30 -1 -1 1",
    reward_row: str = "0 0 -1 -1 0 0 -1",
    extensions: str = "D\n-1 -1 -1 -1\nS\n",
) -> str:
  return (
      f"#{vnum}\nQuest {vnum}~\nA test quest.~\nAccepted.~\nCompleted.~\nAbandoned.~\n"
      f"0 100 {flags} 100 -1 -1 -1\n{row_two}\n{reward_row}\n{extensions}"
  )


class QuestContractTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.repo_root = default_repo_root()
    cls.fixtures = cls.repo_root / "scripts/world/tests/fixtures/phase2"
    cls.manifest = load_manifest()
    cls.config = {"diagonal_dirs": False, "config_source": "test", "assumed": False}

  def read_lines(self, relative_path: str) -> list[str]:
    path = self.fixtures / relative_path
    content = path.read_bytes()
    self.assertNotIn(b"\r", content)
    content.decode("ascii")
    return content.decode("ascii").splitlines()

  def test_canonical_writer_contract_sample_has_current_rows(self) -> None:
    lines = self.read_lines("contracts/qst/canonical.qst")
    self.assertEqual("#10100", lines[0])
    self.assertEqual(7, len(lines[6].split()))
    self.assertEqual(7, len(lines[7].split()))
    self.assertEqual(7, len(lines[8].split()))
    self.assertEqual("D", lines[9])
    self.assertEqual(4, len(lines[10].split()))
    self.assertEqual("S", lines[11])
    self.assertEqual("$~", lines[12])

    path = self.fixtures / "contracts/qst/canonical.qst"
    result = parse_quest_file(path, "canonical.qst", self.manifest)
    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    self.assertEqual(1, len(result.records))
    record = result.records[0]
    self.assertEqual(10100, record.vnum)
    self.assertEqual("Canonical Quest", record.name)
    self.assertEqual(0, record.quest_type)
    self.assertEqual(10000, record.questmaster_vnum)
    self.assertEqual(20000, record.target)
    self.assertIsNone(record.previous_quest_vnum)
    self.assertEqual(7, record.reward_row_width)
    self.assertEqual(1, record.dialogue_block_count)
    self.assertEqual(-1, record.raw_values["reward_object_vnum"])
    self.assertIsNone(record.reward_object_vnum)
    self.assertGreater(record.field_spans["questmaster_vnum"].column, 1)

  def test_legacy_contract_sample_preserves_loader_compatibility(self) -> None:
    lines = self.read_lines("contracts/qst/legacy.qst")
    self.assertEqual(3, len(lines[8].split()))
    self.assertNotIn("D", lines)
    self.assertEqual("S", lines[-2])
    self.assertEqual("$", lines[-1])

    path = self.fixtures / "contracts/qst/legacy.qst"
    result = parse_quest_file(path, "legacy.qst", self.manifest)
    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    record = result.records[0]
    self.assertEqual(3, record.reward_row_width)
    self.assertIsNone(record.race_reward)
    self.assertIsNone(record.wilderness_x)
    self.assertIsNone(record.wilderness_y)
    self.assertIsNone(record.follower_mobile_vnum)
    self.assertNotIn("race_reward", record.raw_values)
    self.assertEqual(0, record.dialogue_block_count)

  def test_complete_fixture_has_normal_and_mini_qst_packages(self) -> None:
    complete = self.fixtures / "complete/qst"
    self.assertEqual("100.qst\n$\n", (complete / "index").read_text(encoding="ascii"))
    self.assertEqual(
        (complete / "index").read_bytes(),
        (complete / "index.mini").read_bytes(),
    )
    lines = self.read_lines("complete/qst/100.qst")
    self.assertEqual(["#10000", "#10001"], [line for line in lines if line.startswith("#")])

    parsed = parse_quest_file(complete / "100.qst", "qst/100.qst", self.manifest)
    self.assertTrue(parsed.complete)
    self.assertEqual([], parsed.findings)
    self.assertEqual([10000, 10001], [record.vnum for record in parsed.records])
    self.assertEqual({0}, parsed.records[0].flag_bits)
    self.assertEqual(10001, parsed.records[0].next_quest_vnum)
    self.assertEqual(10000, parsed.records[1].previous_quest_vnum)

  def test_multiline_strings_comments_and_blank_lines_use_shared_cursor_rules(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "120.qst"
      content = quest_record(12000).replace(
          "A test quest.~\nAccepted.~",
          "First description line\nSecond description line~\nAccepted.~",
      ).replace(
          "1 0 1 30 -1 -1 1\n",
          "* row-two comment\n\n1 0 1 30 -1 -1 1\n",
      )
      path.write_text(content + "$~\n", encoding="ascii")
      result = parse_quest_file(path, "120.qst", self.manifest)
      self.assertTrue(result.complete)
      self.assertEqual([], result.findings)
      self.assertEqual(
          "First description line\r\nSecond description line",
          result.records[0].description,
      )

  def test_short_numeric_row_recovers_at_next_credible_header(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "121.qst"
      path.write_text(
          quest_record(12100, row_two="1 2 3") + quest_record(12101) + "$~\n",
          encoding="ascii",
      )
      result = parse_quest_file(path, "121.qst", self.manifest)
      self.assertFalse(result.complete)
      self.assertEqual([12100, 12101], [record.vnum for record in result.records])
      self.assertFalse(result.records[0].complete)
      self.assertTrue(result.records[1].complete)
      self.assertIn("QST010", {finding.code for finding in result.findings})

  def test_integer_overflow_and_invalid_flag_are_source_located(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "122.qst"
      path.write_text(
          quest_record(12200, flags="?", row_two="2147483648 0 1 30 -1 -1 1") + "$~\n",
          encoding="ascii",
      )
      result = parse_quest_file(path, "122.qst", self.manifest)
      codes = {finding.code for finding in result.findings}
      self.assertTrue({"QST009", "QST010"} <= codes)
      flag_finding = next(finding for finding in result.findings if finding.code == "QST009")
      self.assertGreater(flag_finding.span.column, 1)

  def test_overlong_flag_token_is_boot_fatal_and_recovers(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "126.qst"
      path.write_text(
          quest_record(12600, flags="a" * 128) + quest_record(12601) + "$~\n",
          encoding="ascii",
      )
      result = parse_quest_file(path, "126.qst", self.manifest)
      self.assertFalse(result.complete)
      self.assertEqual([12600, 12601], [record.vnum for record in result.records])
      self.assertFalse(result.records[0].complete)
      self.assertTrue(result.records[1].complete)
      self.assertIn("QST009", {finding.code for finding in result.findings})

  def test_sscanf_trailing_data_is_diagnosed_without_losing_parsed_values(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "127.qst"
      content = quest_record(12700)
      content = content.replace(
          "0 100 0 100 -1 -1 -1\n",
          "0 100 0 100 -1 -1 -1 ignored\n",
      ).replace(
          "1 0 1 30 -1 -1 1\n",
          "1 0 1 30 -1 -1 1ignored\n",
      )
      path.write_text(content + "$~\n", encoding="ascii")
      result = parse_quest_file(path, "127.qst", self.manifest)
      self.assertTrue(result.complete)
      self.assertEqual({"QST008", "QST010"}, {finding.code for finding in result.findings})
      self.assertEqual(1, result.records[0].quantity)
      self.assertEqual(-1, result.records[0].raw_values["prerequisite_object_vnum"])

  def test_four_to_six_reward_conversions_are_rejected(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "128.qst"
      path.write_text(
          quest_record(12800, reward_row="0 0 -1 -1") + quest_record(12801) + "$~\n",
          encoding="ascii",
      )
      result = parse_quest_file(path, "128.qst", self.manifest)
      self.assertFalse(result.complete)
      self.assertEqual([12800, 12801], [record.vnum for record in result.records])
      self.assertIn("QST011", {finding.code for finding in result.findings})

  def test_unterminated_string_reports_source_and_file_incompleteness(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "129.qst"
      path.write_bytes(b"#12900\nunterminated quest name\n")
      result = parse_quest_file(path, "129.qst", self.manifest)
      self.assertFalse(result.complete)
      self.assertEqual({"QST002", "QST015"}, {finding.code for finding in result.findings})
      self.assertFalse(result.records[0].complete)

  def test_repeated_dialogue_block_preserves_last_runtime_values(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "123.qst"
      extensions = "D\n1 2 3 4\nD\n5 6 7 8\nS\n"
      path.write_text(quest_record(12300, extensions=extensions) + "$~\n", encoding="ascii")
      result = parse_quest_file(path, "123.qst", self.manifest)
      self.assertTrue(result.complete)
      self.assertIn("QST016", {finding.code for finding in result.findings})
      record = result.records[0]
      self.assertEqual(2, record.dialogue_block_count)
      self.assertEqual((5, 6, 7, 8), (
          record.diplomacy_dc,
          record.intimidate_dc,
          record.bluff_dc,
          record.dialogue_alternative_quest_vnum,
      ))

  def test_unknown_extension_and_missing_s_recover_without_stale_records(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "124.qst"
      path.write_text(
          quest_record(12400, extensions="X\nignored payload\n")
          + quest_record(12401, extensions="")
          + quest_record(12402)
          + "$~\n",
          encoding="ascii",
      )
      result = parse_quest_file(path, "124.qst", self.manifest)
      self.assertEqual([12400, 12401, 12402], [record.vnum for record in result.records])
      self.assertEqual({"QST013", "QST014"}, {finding.code for finding in result.findings})
      self.assertFalse(result.records[0].complete)
      self.assertFalse(result.records[1].complete)
      self.assertTrue(result.records[2].complete)

  def test_missing_file_terminator_is_incomplete_and_read_only(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory)
      path = root / "125.qst"
      path.write_text(quest_record(12500), encoding="ascii")
      before = tree_hash(root)
      result = parse_quest_file(path, "125.qst", self.manifest)
      self.assertFalse(result.complete)
      self.assertEqual({"QST015"}, {finding.code for finding in result.findings})
      self.assertEqual(before, tree_hash(root))

  def test_duplicate_order_and_package_findings_use_qst_contract(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory)
      zone = root / "120.zon"
      quest = root / "120.qst"
      misplaced = root / "121.qst"
      zone.write_text(
          "#120\nBuilder~\nQuest package test~\n12000 12099 30 2\nS\n$\n",
          encoding="ascii",
      )
      quest.write_text(
          quest_record(12001) + quest_record(12000) + quest_record(12000) + "$~\n",
          encoding="ascii",
      )
      misplaced.write_text(quest_record(12002) + "$~\n", encoding="ascii")
      result = validate_explicit_paths(
          [zone, quest, misplaced], self.repo_root, self.manifest, self.config
      )
      codes = {finding.code for finding in result.findings}
      self.assertTrue({"QST040", "QST041", "REF010"} <= codes)

  def test_record_order_restarts_at_each_source_package(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory)
      high_package = root / "1191.qst"
      low_package = root / "120.qst"
      high_package.write_text(quest_record(119100) + "$~\n", encoding="ascii")
      low_package.write_text(quest_record(12000) + "$~\n", encoding="ascii")
      result = validate_explicit_paths(
          [high_package, low_package], self.repo_root, self.manifest, self.config
      )
      self.assertNotIn("QST041", {finding.code for finding in result.findings})

  def test_indexed_normal_mini_and_selected_unindexed_qst_loading(self) -> None:
    complete = self.fixtures / "complete"
    normal = load_indexed_world_data(complete, self.repo_root, self.manifest, self.config)
    mini = load_indexed_world_data(
        complete, self.repo_root, self.manifest, self.config, mini=True
    )
    self.assertEqual([10000, 10001], [record.vnum for record in normal.quests])
    self.assertEqual([10000, 10001], [record.vnum for record in mini.quests])

    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      (root / "zon/2.zon").write_text(
          "#2\nBuilder~\nSelected quest zone~\n200 299 30 2\nS\n$\n",
          encoding="ascii",
      )
      (root / "qst/2.qst").write_text(quest_record(200, flags="?") + "$~\n", encoding="ascii")
      status, stdout, stderr = run_cli(
          ["--world-root", str(root), "--json", "validate", "--zone", "2"]
      )
      self.assertEqual(1, status)
      self.assertEqual("", stderr)
      payload = json.loads(stdout)
      codes = {finding["code"] for finding in payload["findings"]}
      self.assertTrue({"IDX008", "QST009"} <= codes)

  def test_explicit_qst_only_uses_an_isolated_reference_universe(self) -> None:
    path = self.fixtures / "contracts/qst/canonical.qst"
    before = path.read_bytes()
    result = validate_explicit_paths([path], self.repo_root, self.manifest, self.config)
    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    self.assertEqual(before, path.read_bytes())

  def test_cli_json_reports_parser_incompleteness_deterministically(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      (root / "qst/1.qst").write_text(
          quest_record(100, row_two="1 2") + "$~\n",
          encoding="ascii",
      )
      arguments = ["--world-root", str(root), "--json", "validate", "--all"]
      first = run_cli(arguments)
      second = run_cli(arguments)
      self.assertEqual(first, second)
      self.assertEqual(1, first[0])
      payload = json.loads(first[1])
      self.assertFalse(payload["complete"])
      self.assertIn("QST010", {finding["code"] for finding in payload["findings"]})
      human_arguments = ["--world-root", str(root), "validate", "--all"]
      human_first = run_cli(human_arguments)
      human_second = run_cli(human_arguments)
      self.assertEqual(human_first, human_second)
      self.assertEqual(1, human_first[0])
      self.assertIn("QST010", human_first[1])
      self.assertIn("parse incomplete", human_first[1])


if __name__ == "__main__":
  unittest.main()
