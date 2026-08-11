from __future__ import annotations

import unittest

from wtool_lib.rol_pilot import package_metrics, pilot_coverage


def record(basename: str, kind: str, vnum: int, directives: list[dict], mode=None) -> dict:
  row = {
      "basename": basename,
      "kind": kind,
      "vnum": vnum,
      "record_id": f"{kind}:{vnum}:{basename}:1",
      "directives": directives,
  }
  if mode is not None:
    row["format_version"] = mode
  return row


def action(source: dict, name: str = "ADD") -> dict:
  return {
      "basename": source["basename"],
      "source_record_id": source["record_id"],
      "action": name,
  }


class RolPilotTests(unittest.TestCase):
  def test_package_metrics_counts_modes_codes_resets_and_actions(self) -> None:
    records = [
        record(
            "muspel",
            "soc",
            1,
            [{"token": "ACTION", "arguments": [1004]}],
            "PATH",
        ),
        record(
            "muspel",
            "zon",
            2,
            [{"token": "F"}, {"token": "X"}],
        ),
    ]
    metrics = package_metrics(
        "muspel",
        records,
        [action(records[0]), action(records[1], "MERGE")],
        52,
    )
    self.assertEqual(2, metrics["records"])
    self.assertEqual({"ADD": 1, "MERGE": 1}, metrics["actions"])
    self.assertEqual({"PATH": 1}, metrics["soc_modes"])
    self.assertEqual([1004], metrics["soc_special_codes"])
    self.assertEqual(["F", "X"], metrics["zone_reset_tokens"])
    self.assertEqual(52, metrics["source_special_bindings"])

  def test_complete_selection_covers_every_phase4_category(self) -> None:
    packages = [
        {
            "basename": "swamp_two",
            "zone_reset_tokens": ["D", "E", "G", "M", "O", "P"],
            "records_by_kind": {"zon": 1},
            "soc_modes": {},
            "soc_special_codes": [],
            "uncommon_extensions": ["obj:AFFECT_FLAGS"],
            "source_special_bindings": 0,
            "actions": {"ADD": 1},
        },
        {
            "basename": "hulburg",
            "zone_reset_tokens": ["F"],
            "records_by_kind": {"qst": 1, "shp": 1},
            "soc_modes": {"LIST": 1},
            "soc_special_codes": [],
            "uncommon_extensions": [],
            "source_special_bindings": 10,
            "actions": {"KEEP": 1},
        },
        {
            "basename": "muspel",
            "zone_reset_tokens": ["F", "X"],
            "records_by_kind": {"soc": 5},
            "soc_modes": {name: 1 for name in ("PATH", "PERIODIC", "TIMED", "TRIGGER")},
            "soc_special_codes": [1001, 1003, 1004],
            "uncommon_extensions": [],
            "source_special_bindings": 52,
            "actions": {"ADD": 5},
        },
        {
            "basename": "theswamp",
            "zone_reset_tokens": ["T"],
            "records_by_kind": {"soc": 1},
            "soc_modes": {},
            "soc_special_codes": [1000],
            "uncommon_extensions": [],
            "source_special_bindings": 3,
            "actions": {"ADD": 1},
        },
        {
            "basename": "cemetery",
            "zone_reset_tokens": [],
            "records_by_kind": {"soc": 1},
            "soc_modes": {},
            "soc_special_codes": [1002],
            "uncommon_extensions": ["wld:R"],
            "source_special_bindings": 13,
            "actions": {"ADD": 1},
        },
    ]
    coverage = pilot_coverage(packages)
    self.assertTrue(coverage["complete"])
    self.assertTrue(all(coverage["checks"].values()))

  def test_missing_timed_mode_fails_selection(self) -> None:
    packages = [
        {
            "basename": "swamp_two",
            "zone_reset_tokens": ["D", "E", "G", "M", "O", "P"],
            "records_by_kind": {},
            "soc_modes": {},
            "soc_special_codes": [],
            "uncommon_extensions": [],
            "source_special_bindings": 0,
            "actions": {},
        },
        {
            "basename": "hulburg",
            "zone_reset_tokens": [],
            "records_by_kind": {"qst": 1, "shp": 1},
            "soc_modes": {"LIST": 1},
            "soc_special_codes": [],
            "uncommon_extensions": [],
            "source_special_bindings": 0,
            "actions": {"KEEP": 1},
        },
        {
            "basename": "muspel",
            "zone_reset_tokens": ["F", "T", "X"],
            "records_by_kind": {},
            "soc_modes": {name: 1 for name in ("PATH", "PERIODIC", "TRIGGER")},
            "soc_special_codes": [1000, 1001, 1002, 1003, 1004],
            "uncommon_extensions": ["obj:AFFECT_FLAGS", "wld:R"],
            "source_special_bindings": 30,
            "actions": {},
        },
    ]
    self.assertFalse(pilot_coverage(packages)["checks"]["all_soc_modes"])


if __name__ == "__main__":
  unittest.main()
