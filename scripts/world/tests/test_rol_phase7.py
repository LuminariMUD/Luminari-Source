from __future__ import annotations

import json
from pathlib import Path
import tempfile
import unittest

from wtool_lib.rol_phase7 import (
    _bound_trigger_text,
    _ensure_index_entry,
    _filter_zone_door_resets,
    _merge_hlquest_blocks,
    _patch_record_block,
    _typed_resolver,
    _validation_delta,
    RolPhase7Error,
)


class RolPhase7Tests(unittest.TestCase):
  def test_hlquest_merge_keeps_one_host_and_deduplicates_exact_bodies(self) -> None:
    first = "#2000100\nA!\nhello~\nworld~\n"
    second = "#2000100\nQ!\ndone~\nS\n"

    merged = _merge_hlquest_blocks(
        [
            (2_000_100, first, "first"),
            (2_000_100, first, "duplicate"),
            (2_000_100, second, "second"),
        ]
    )

    self.assertEqual(1, merged.count("#2000100\n"))
    self.assertEqual(1, merged.count("A!\n"))
    self.assertEqual(1, merged.count("Q!\n"))

  def test_mobile_patch_places_spec_proc_inside_enhanced_section(self) -> None:
    source = (
        "#2000100\nmobile~\na mobile~\nA mobile is here.~\nA mobile.~\n"
        "0 0 0 0 0 0 0 0 0 E\n1 0 0 1d1+0 1d1+0\n0 0\n9 9 0\n"
        "Class: 0\nRace: 0\nE\n"
    )

    patched = _patch_record_block(
        "mob", 2_000_100, source, "RoL Composite Mobile", (2_026_167,), (), ()
    )

    self.assertLess(patched.index("SpecProc:"), patched.rindex("\nE\n"))
    self.assertGreater(patched.index("T 2026167"), patched.rindex("\nE\n"))

  def test_door_filter_excludes_only_known_absent_directions(self) -> None:
    source = "D 0 2000100 1 1\nD 0 2000100 2 1\nD 0 2999999 4 1\n"

    filtered, diagnostics = _filter_zone_door_resets(source, {2_000_100: {1}})

    self.assertIn("2000100 1", filtered)
    self.assertNotIn("2000100 2", filtered)
    self.assertIn("2999999 4", filtered)
    self.assertEqual(1, len(diagnostics))

  def test_trigger_echo_lines_are_bounded_without_dropping_text(self) -> None:
    payload = "word " * 180
    source = f"  mecho {payload}\n"

    bounded, diagnostics = _bound_trigger_text(2_026_167, source)

    self.assertTrue(all(len(line.encode("ascii")) <= 480 for line in bounded.splitlines()))
    self.assertGreater(bounded.count("mecho "), 1)
    self.assertEqual(1, len(diagnostics))

  def test_validation_delta_treats_same_record_reclassification_as_inherited(self) -> None:
    baseline = {
        "findings": [
            {
                "severity": "error",
                "code": "REF022",
                "message": "missing object",
                "path": "wld/1.wld",
                "record_type": "room",
                "vnum": 100,
            }
        ]
    }
    staged = {
        "findings": [
            {
                "severity": "error",
                "code": "REF023",
                "message": "wrong typed object",
                "path": "wld/1.wld",
                "record_type": "room",
                "vnum": 100,
            }
        ]
    }

    delta = _validation_delta(baseline, staged)

    self.assertEqual(0, delta["new_active_errors"])
    self.assertEqual([], delta["new_findings"])

  def test_index_updates_are_numeric_and_idempotent(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      path = Path(temporary) / "index"
      path.write_text("12157521.wld\n10.wld\n$\n", encoding="ascii")

      self.assertTrue(_ensure_index_entry(path, "20000.wld"))
      self.assertFalse(_ensure_index_entry(path, "20000.wld"))
      self.assertEqual(
          ["10.wld", "20000.wld", "12157521.wld", "$"],
          path.read_text(encoding="ascii").splitlines(),
      )

  def test_typed_resolver_never_falls_back_to_luminari_exact_identity(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      plan = Path(temporary)
      (plan / "identity-map.jsonl").write_text(
          json.dumps(
              {
                  "source_kind": "obj",
                  "source_vnum": 14017,
                  "destination_vnum": 2_014_017,
              }
          )
          + "\n",
          encoding="ascii",
      )
      resolve = _typed_resolver(
          plan,
          [
              {
                  "resolution": "target_exact",
                  "target_type": "object",
                  "target_vnum": 14018,
              }
          ],
      )

      self.assertEqual(2_014_017, resolve("obj", 14017))
      with self.assertRaisesRegex(RolPhase7Error, "no typed identity for obj 14018"):
        resolve("obj", 14018)


if __name__ == "__main__":
  unittest.main()
