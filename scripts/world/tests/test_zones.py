from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from wtool_lib.constants import default_repo_root, load_manifest
from wtool_lib.zones import parse_zone_file


class ZoneParserTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.repo_root = default_repo_root()
    cls.fixture_root = cls.repo_root / "scripts/world/tests/fixtures/phase1"
    cls.manifest = load_manifest()

  def parse(self, name: str):
    path = self.fixture_root / name
    return parse_zone_file(path, path.relative_to(self.repo_root).as_posix(), self.manifest, 6)

  def test_valid_zone_covers_current_reset_forms(self) -> None:
    result = self.parse("valid/100.zon")
    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    self.assertEqual(list("MTVGEOTVDRJ"), [command.command for command in result.records[0].commands])

  def test_header_truncation_field_count_is_rejected(self) -> None:
    result = self.parse("broken/header-truncation.zon")
    self.assertIn("ZON004", {finding.code for finding in result.findings})
    self.assertFalse(result.complete)

  def test_prescan_whitespace_mismatch_is_rejected(self) -> None:
    result = self.parse("broken/prescan.zon")
    self.assertIn("ZON020", {finding.code for finding in result.findings})

  def test_dependency_and_i_l_v_traps_are_diagnosed_together(self) -> None:
    result = self.parse("broken/reset-traps.zon")
    codes = {finding.code for finding in result.findings}
    self.assertTrue({"ZON023", "ZON033", "ZON034", "ZON035"} <= codes)

  def test_r_uses_harmless_effective_three_integer_form(self) -> None:
    result = self.parse("valid/100.zon")
    remove = next(command for command in result.records[0].commands if command.command == "R")
    self.assertEqual([10000, 30000], remove.arguments)
    self.assertNotIn("ZON021", {finding.code for finding in result.findings})

  def test_invalid_effective_reset_direction_is_rejected(self) -> None:
    result = self.parse("broken/invalid-directions.zon")
    self.assertIn("ZON029", {finding.code for finding in result.findings})

  def test_luminari_region_faction_and_city_ranges_are_enforced(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "123.zon"
      path.write_text(
          "#123\nBuilder~\nRanges~\n"
          "12300 12399 30 2 0 0 0 0 1 20 1 14 4 3\nS\n$\n",
          encoding="ascii",
      )
      result = parse_zone_file(path, "123.zon", self.manifest, 6)
      codes = {finding.code for finding in result.findings}
      self.assertTrue({"ZON015", "ZON016", "ZON017"} <= codes)

  def test_second_zone_record_is_rejected(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "119.zon"
      path.write_text(
          "#119\nBuilder~\nFirst~\n11900 11949 30 2\nS\n$\n"
          "#120\nBuilder~\nSecond~\n12000 12049 30 2\nS\n$\n",
          encoding="ascii",
      )
      result = parse_zone_file(path, "119.zon", self.manifest, 6)
      self.assertFalse(result.complete)
      self.assertIn("ZON044", {finding.code for finding in result.findings})

  def test_overlong_prescan_comment_is_not_silently_skipped(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "124.zon"
      path.write_bytes(
          b"*" + b"x" * 510 + b"\n"
          b"#124\nBuilder~\nLong Comment~\n12400 12499 30 2\nS\n$\n"
      )
      result = parse_zone_file(path, "124.zon", self.manifest, 6)
      self.assertIn("ZON001", {finding.code for finding in result.findings})


if __name__ == "__main__":
  unittest.main()
