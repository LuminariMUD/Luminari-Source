from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from wtool_lib.models import SourceSpan, WorldData, ZoneRecord
from wtool_lib.rol_discovery import (
    build_capability_matrix,
    build_target_catalog,
    extract_source_commands,
    lineage_candidates,
    resolve_reference,
)
from wtool_lib.rol_source import RolRecord, RolReference, RolSourceCorpus


class RolDiscoveryTests(unittest.TestCase):
  def test_documented_formula_and_identity_seed_is_confirmed(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      world_root = Path(temporary)
      (world_root / "zon").mkdir()
      (world_root / "zon/1507.zon").write_text("#1507\n", encoding="ascii")
      target = ZoneRecord(
          1507,
          SourceSpan("zon/1507.zon", 1),
          "1507",
          name="Hulburg Trail",
      )
      catalog = build_target_catalog(WorldData(zones=[target]), world_root)
      source = RolRecord(
          "zon",
          507,
          "trail",
          "areas/zon/trail.zon",
          1,
          10,
          "a" * 64,
          identity="Hulburg Trail",
      )
      row = lineage_candidates(source, catalog, {})
      self.assertEqual("candidates", row["candidate_state"])
      self.assertTrue(row["candidates"][0]["confirmed_seed"])
      self.assertEqual(
          ["documented_traced_seed", "exact_normalized_identity", "legacy_offset_formula"],
          row["candidates"][0]["evidence"],
      )

  def test_reference_resolution_distinguishes_active_excluded_and_target(self) -> None:
    reference = RolReference("mobile", 100, "test", "sample", 1)
    self.assertEqual(
        ("active_source", "map_active_definition"),
        resolve_reference(reference, {("mobile", 100)}, set(), set(), set(), set()),
    )
    self.assertEqual(
        ("excluded_source", "exclude_dependent_instruction"),
        resolve_reference(
            reference,
            {("mobile", 100)},
            {("mobile", 100)},
            set(),
            set(),
            set(),
        ),
    )
    self.assertEqual(
        ("target_lineage_candidate", "resolve_lineage_before_emission"),
        resolve_reference(reference, set(), set(), set(), {("mobile", 100100)}, set()),
    )

  def test_capability_rows_are_counted_and_owned(self) -> None:
    record = RolRecord(
        "zon",
        1,
        "sample",
        "areas/zon/sample.zon",
        1,
        2,
        "b" * 64,
        directives=[{"token": "F", "line": 2}],
    )
    rows = build_capability_matrix(RolSourceCorpus(records=[record]))
    self.assertEqual("A", rows[0]["classification"])
    self.assertEqual("owned", rows[0]["status"])

  def test_source_command_inventory_includes_special_actions(self) -> None:
    repo_root = Path(__file__).resolve().parents[3]
    source_root = repo_root / "EXAMPLE/RealmsOfLuminari"
    result = extract_source_commands(source_root)
    self.assertGreater(result["command_count"], 100)
    self.assertEqual(
        list(range(1000, 1005)),
        [entry["action_code"] for entry in result["special_actions"]],
    )


if __name__ == "__main__":
  unittest.main()
