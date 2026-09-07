from __future__ import annotations

from collections import defaultdict
import json
from pathlib import Path
import tempfile
import tracemalloc
import unittest

from wtool_lib.rol_planner import _load_jsonl, build_record_actions, confirmed_lineage_packages
from wtool_lib.rol_pilot_build import _load_jsonl as load_pilot_jsonl


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
  def test_repeated_lineage_evidence_loads_with_bounded_string_memory(self) -> None:
    candidate = {
        "target_type": "room",
        "target_vnum": 2100100,
        "path": "wld/21001.wld",
        "line": 12,
        "record_sha256": "a" * 64,
        "source_file_sha256": "b" * 64,
        "score": 50,
        "confirmed_seed": False,
        "evidence": ["exact_normalized_identity"],
    }
    with tempfile.TemporaryDirectory() as temporary:
      path = Path(temporary) / "candidates.jsonl"
      path.write_text("".join(
          json.dumps({"source_record_id": f"wld:{index}", "candidates": [candidate] * 40}) + "\n"
          for index in range(200)
      ), encoding="ascii")

      def measure(loader):
        tracemalloc.start()
        try:
          rows = loader(path)
          peak = tracemalloc.get_traced_memory()[1]
        finally:
          tracemalloc.stop()
        return rows, peak

      def ordinary_load(source):
        with source.open(encoding="ascii") as stream:
          return [json.loads(line) for line in stream]

      expected, ordinary_peak = measure(ordinary_load)
      for loader in (_load_jsonl, load_pilot_jsonl):
        with self.subTest(loader=loader.__module__):
          rows, peak = measure(loader)
          self.assertEqual(expected, rows)
          self.assertLess(peak, ordinary_peak * 0.8)
          # Sharing immutable text must not alias mutable candidate data.
          rows[0]["candidates"][0]["path"] = "changed"
          rows[0]["candidates"][0]["evidence"].append("changed")
          self.assertEqual(candidate, rows[0]["candidates"][1])
          self.assertEqual(candidate, rows[1]["candidates"][0])

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
        "evidence": ["legacy_lineage_formula"],
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
          "evidence": ["exact_normalized_identity", "legacy_lineage_formula"],
          "confirmed_seed": index == 0,
      }
      candidates[record["record_id"]] = candidate_row(record, candidate)
    confirmed = confirmed_lineage_packages(records, candidates)
    self.assertIn("lineage", confirmed)
    self.assertEqual(1.0, confirmed["lineage"]["legacy_formula_ratio"])

  def test_historic_equivalents_do_not_override_distinct_canonical_targets(self) -> None:
    first = source_record("obj:1007:x:1", "obj", 1007, "artifacts")
    second = source_record("obj:1009:x:2", "obj", 1009, "artifacts")
    self.policy["identity"]["historic_target_lineage"] = [
        {
            "source_kind": "obj",
            "source_vnum": source_vnum,
            "target_type": "object",
            "historic_target_vnum": 169906,
            "evidence": ["reviewed artifact contract"],
        }
        for source_vnum in (1007, 1009)
    ]

    actions, identities, _ = build_record_actions(
        [first, second],
        [candidate_row(first), candidate_row(second)],
        self.policy,
        defaultdict(dict),
    )

    self.assertEqual(["ADD", "ADD"], [row["action"] for row in actions])
    self.assertEqual([2001007, 2001009], [row["destination_vnum"] for row in actions])
    self.assertTrue(all(row["selected_target"] is None for row in actions))
    self.assertEqual([2001007, 2001009], [row["destination_vnum"] for row in identities])


if __name__ == "__main__":
  unittest.main()
