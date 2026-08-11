from __future__ import annotations

import hashlib
from pathlib import Path
import tempfile
import unittest

from wtool_lib.constants import default_repo_root, load_manifest
from wtool_lib.rooms import parse_room_file
from wtool_lib.spec_registry import extract_spec_names
from wtool_lib.world import validate_explicit_paths


def directory_hash(root: Path) -> str:
  digest = hashlib.sha256()
  for path in sorted(item for item in root.rglob("*") if item.is_file()):
    digest.update(path.relative_to(root).as_posix().encode("ascii"))
    digest.update(path.read_bytes())
  return digest.hexdigest()


class RoomParserTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.repo_root = default_repo_root()
    cls.fixture_root = cls.repo_root / "scripts/world/tests/fixtures/phase1"
    cls.manifest = load_manifest()
    cls.spec_names = extract_spec_names(cls.repo_root)

  def parse(self, name: str, diagonal_dirs: bool = False):
    path = self.fixture_root / name
    return parse_room_file(
        path,
        path.relative_to(self.repo_root).as_posix(),
        self.manifest,
        diagonal_dirs,
        self.spec_names,
    )

  def test_valid_bundle_parses_all_room_extensions_and_is_read_only(self) -> None:
    root = self.fixture_root / "valid"
    before = directory_hash(root)
    config = {"diagonal_dirs": False, "config_source": "test", "assumed": False}
    result = validate_explicit_paths([root], self.repo_root, self.manifest, config)
    self.assertEqual({"SEM010"}, {item.code for item in result.findings})
    self.assertTrue(result.complete)
    self.assertEqual(before, directory_hash(root))

  def test_malformed_coordinates_and_exit_values_recover_in_one_record(self) -> None:
    result = self.parse("broken/room-structure.wld")
    codes = {finding.code for finding in result.findings}
    self.assertTrue({"WLD014", "WLD016"} <= codes)
    self.assertEqual(1, len(result.records))

  def test_modern_flags_line_accepts_a_trailing_loader_annotation(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "122.wld"
      path.write_text(
          "#12200\nAnnotated~\nThe flags line has trailing text.~\n"
          "122 0 0 0 0 0 ignored annotation\nS\n$~\n",
          encoding="ascii",
      )
      result = parse_room_file(path, "122.wld", self.manifest, False, self.spec_names)
      self.assertEqual([], result.findings)

  def test_level_range_record_parses_and_rejects_malformed_or_duplicate_rows(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "123.wld"
      path.write_text(
          "#12300\nValid~\nA bounded room.~\n123 0 0 0 0 0\nR 15 -1\nS\n"
          "#12301\nBroken~\nA malformed range.~\n123 0 0 0 0 0\n"
          "R 1 10 trailing\nR 1 10\nR 2 9\nS\n$~\n",
          encoding="ascii",
      )
      result = parse_room_file(path, "123.wld", self.manifest, False, self.spec_names)
      self.assertEqual((15, -1), (result.records[0].minimum_level, result.records[0].maximum_level))
      self.assertEqual(2, sum(item.code == "WLD036" for item in result.findings))

  def test_disabled_diagonal_consumes_block_but_reports_desync(self) -> None:
    disabled = self.parse("broken/diagonal.wld", diagonal_dirs=False)
    enabled = self.parse("broken/diagonal.wld", diagonal_dirs=True)
    self.assertIn("WLD011", {finding.code for finding in disabled.findings})
    self.assertNotIn("WLD011", {finding.code for finding in enabled.findings})
    self.assertEqual(2, len(disabled.records))

  def test_invalid_directions_are_reported_for_exit_and_moving_room(self) -> None:
    result = self.parse("broken/invalid-directions.wld", diagonal_dirs=True)
    codes = {finding.code for finding in result.findings}
    self.assertTrue({"WLD010", "WLD021", "WLD026"} <= codes)

  def test_moving_room_clamp_and_expanded_boundary_are_diagnosed(self) -> None:
    result = self.parse("broken/moving-limit.wld")
    codes = {finding.code for finding in result.findings}
    self.assertTrue({"WLD025", "WLD028"} <= codes)
    self.assertEqual(150, sum(item.repeat for item in result.records[0].moving_connections))

  def test_out_of_order_rooms_are_caught_across_graph_load_order(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      zone = Path(directory) / "116.zon"
      zone.write_text("#116\nBuilder~\nOrder~\n11600 11699 30 2\nS\n$\n", encoding="ascii")
      room = self.fixture_root / "broken/out-of-order.wld"
      config = {"diagonal_dirs": False, "config_source": "test", "assumed": False}
      result = validate_explicit_paths([zone, room], self.repo_root, self.manifest, config)
      self.assertIn("WLD041", {finding.code for finding in result.findings})

  def test_dangling_exit_is_reported_by_graph_pass(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      zone = Path(directory) / "113.zon"
      zone.write_text("#113\nBuilder~\nRefs~\n11300 11399 30 2\nS\n$\n", encoding="ascii")
      room = self.fixture_root / "broken/room-structure.wld"
      config = {"diagonal_dirs": False, "config_source": "test", "assumed": False}
      result = validate_explicit_paths([zone, room], self.repo_root, self.manifest, config)
      self.assertIn("REF003", {finding.code for finding in result.findings})

  def test_unterminated_room_string_marks_parse_incomplete(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "117.wld"
      path.write_bytes(b"#11700\nunterminated name\n")
      result = parse_room_file(path, "117.wld", self.manifest, False, self.spec_names)
      self.assertFalse(result.complete)
      self.assertEqual({"WLD002"}, {finding.code for finding in result.findings})

  def test_missing_room_sentinel_recovers_at_next_record_header(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "121.wld"
      path.write_text(
          "#12100\nFirst~\nThe first room lacks S.~\n121 0 0 0 0 0\n"
          "#12101\nSecond~\nThe second room is intact.~\n121 0 0 0 0 0\nS\n$~\n",
          encoding="ascii",
      )
      result = parse_room_file(path, "121.wld", self.manifest, False, self.spec_names)
      self.assertEqual([12100, 12101], [record.vnum for record in result.records])
      self.assertIn("WLD032", {finding.code for finding in result.findings})

  def test_tracked_zone_room_bundles_parse_without_errors_or_writes(self) -> None:
    config = {"diagonal_dirs": False, "config_source": "test", "assumed": False}
    for name in ("artifacts", "minimal"):
      with self.subTest(bundle=name):
        root = self.repo_root / "lib/world" / name
        before = directory_hash(root)
        phase1_paths = sorted(root.glob("*.zon")) + sorted(root.glob("*.wld"))
        result = validate_explicit_paths(phase1_paths, self.repo_root, self.manifest, config)
        errors = [finding for finding in result.findings if finding.severity == "error"]
        self.assertEqual([], errors)
        self.assertTrue(result.complete)
        self.assertEqual(before, directory_hash(root))

  def test_world_variable_requires_existing_room_script_state(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory)
      zone = root / "120.zon"
      room = root / "120.wld"
      zone.write_text(
          "#120\nBuilder~\nScript State~\n12000 12099 30 2\n"
          "V 0 2 0 12000 fixture enabled\nS\n$\n",
          encoding="ascii",
      )
      room.write_text(
          "#12000\nRoom~\nNo script is attached.~\n120 0 0 0 0 0\nS\n$~\n",
          encoding="ascii",
      )
      config = {"diagonal_dirs": False, "config_source": "test", "assumed": False}
      result = validate_explicit_paths([zone, room], self.repo_root, self.manifest, config)
      self.assertIn("ZON037", {finding.code for finding in result.findings})


if __name__ == "__main__":
  unittest.main()
