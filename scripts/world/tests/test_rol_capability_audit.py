from __future__ import annotations

import json
from pathlib import Path
import tempfile
import unittest

from wtool_lib.constants import default_repo_root
from wtool_lib.rol_capability_audit import (
    build_symbolic_inventory,
    classify_transform_diagnostic,
    write_capability_audit_bundle,
)


class RolCapabilityAuditTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.root = default_repo_root()
    cls.source_root = cls.root / "EXAMPLE/RealmsOfLuminari"
    cls.plan_dir = cls.root / "lib/rol-conversion/runs/phase2-e6ea7982"

  def test_diagnostic_ownership_separates_references_and_generic_gaps(self) -> None:
    self.assertEqual(
        "reference-gap",
        classify_transform_diagnostic("excluded unresolved shop product 10: no identity for obj 10"),
    )
    self.assertEqual(
        "inert-omission",
        classify_transform_diagnostic("omitted obsolete source room mana at source line 2"),
    )
    self.assertEqual(
        "generic-capability-gap",
        classify_transform_diagnostic("room flags without target persistence: [13]"),
    )
    self.assertEqual(
        "bounded-adapter",
        classify_transform_diagnostic("adapted legacy exit trap payload at source line 10"),
    )
    self.assertEqual(
        "source-only-symbol",
        classify_transform_diagnostic(
            "disabled source spell 453 (mud to rock) in magic-item slot 3; "
            "target has no equivalent"
        ),
    )
    self.assertEqual(
        "source-defect",
        classify_transform_diagnostic(
            "excluded completion at source line 29 because a required input cannot be staged"
        ),
    )

  def test_zone_flags_are_owned_by_room_compatibility_or_source_metadata(self) -> None:
    from wtool_lib.rol_source import RolRecord

    record = RolRecord(
        kind="zon",
        vnum=1,
        basename="test",
        path="areas/test.zon",
        line=1,
        end_line=2,
        sha256="0" * 64,
        identity="test",
        values={"header": [199, 30, 2, 0x1FF]},
        directives=[],
    )
    rows = [row for row in build_symbolic_inventory([record]) if row["family"] == "zone_flag"]

    self.assertEqual(list(range(9)), [row["value"] for row in rows])
    self.assertTrue(all(row["mapped"] for row in rows))

  def test_active_corpus_audit_emits_every_convertible_record(self) -> None:
    if not self.source_root.is_dir() or not self.plan_dir.is_dir():
      self.skipTest("ignored RoL corpus or Phase 2 plan is not installed")
    with tempfile.TemporaryDirectory() as temporary:
      output_dir = Path(temporary) / "audit"
      summary = write_capability_audit_bundle(
          self.plan_dir,
          self.source_root,
          output_dir,
          created_at="2026-08-12T00:00:00Z",
      )
      manifest = json.loads((output_dir / "run-manifest.json").read_text(encoding="ascii"))

    self.assertEqual(71_680, summary["source_records"])
    self.assertEqual(69_920, summary["emitted_records"])
    self.assertEqual(0, summary["transform_exceptions"])
    self.assertEqual(0, summary["quest_random_item_ranges"])
    self.assertTrue(manifest["acceptance"]["all_records_disposed"])
    self.assertTrue(manifest["acceptance"]["all_convertible_records_emitted"])
    self.assertEqual(0, manifest["acceptance"]["live_target_writes"])


if __name__ == "__main__":
  unittest.main()
