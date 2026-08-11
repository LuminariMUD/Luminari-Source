from __future__ import annotations

from collections import defaultdict
import unittest

from wtool_lib.rol_planner import build_record_actions, confirmed_lineage_packages


def source_record(record_id: str, kind: str, vnum: int, basename: str) -> dict:
  return {
      "record_id": record_id,
      "kind": kind,
      "vnum": vnum,
      "basename": basename,
      "path": f"areas/{kind}/{basename}.{kind}",
      "line": 1,
      "sha256": "a" * 64,
      "directives": [{"token": "BASE", "line": 2}],
  }


def candidate_row(record: dict, candidate: dict | None = None) -> dict:
  return {
      "source_record_id": record["record_id"],
      "candidate_state": "candidates" if candidate else "explicit_absence",
      "candidates": [candidate] if candidate else [],
  }


class RolPlannerTests(unittest.TestCase):
  def setUp(self) -> None:
    self.policy = {
        "apply": {"permitted_actions": ["KEEP", "PATCH", "ADD", "MERGE", "EXCLUDE"]},
        "identity": {
            "new_entity_range": {"start": 2000000, "end": 2999999, "offset": 2000000},
            "new_zone_range": {"start": 20000, "end": 29999, "offset": 20000},
        },
    }

  def test_ambiguous_candidate_is_preserved_and_record_is_added(self) -> None:
    record = source_record("mob:10:x:1", "mob", 10, "sample")
    candidate = {
        "target_type": "mobile",
        "target_vnum": 100010,
        "evidence": ["legacy_offset_formula"],
        "confirmed_seed": False,
    }
    actions, identities, packages = build_record_actions(
        [record],
        [candidate_row(record, candidate)],
        self.policy,
        defaultdict(dict),
    )
    self.assertEqual("ADD", actions[0]["action"])
    self.assertEqual(2000010, actions[0]["destination_vnum"])
    self.assertIsNone(actions[0]["selected_target"])
    self.assertEqual([], list(packages))
    self.assertEqual("ADD", identities[0]["resolution"])

  def test_confirmed_package_requires_seed_and_broad_evidence(self) -> None:
    records = []
    candidates = {}
    for index in range(20):
      kind = "zon" if index == 0 else "wld"
      record = source_record(f"{kind}:{index}:x:1", kind, index, "lineage")
      records.append(record)
      candidate = {
          "target_type": "zone" if kind == "zon" else "room",
          "target_vnum": index + (1000 if kind == "zon" else 100000),
          "evidence": ["exact_normalized_identity", "legacy_offset_formula"],
          "confirmed_seed": index == 0,
      }
      candidates[record["record_id"]] = candidate_row(record, candidate)
    confirmed = confirmed_lineage_packages(records, candidates)
    self.assertIn("lineage", confirmed)
    self.assertEqual(1.0, confirmed["lineage"]["legacy_formula_ratio"])


if __name__ == "__main__":
  unittest.main()
