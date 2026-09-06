from __future__ import annotations

import json
from contextlib import ExitStack
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from wtool_lib.flags import encode_bits
from wtool_lib.rol_phase8 import (
    _CODE_EVIDENCE_PATHS,
    _action_audit,
    _code_evidence,
    _documentation_audit,
    _line_format_audit,
    _mechanics_isolation_audit,
    _repeat_evidence,
    write_phase8_completion,
)


class RolPhase8Tests(unittest.TestCase):
  def test_code_evidence_captures_format_owners_and_inference_inputs(self) -> None:
    repo_root = Path(__file__).resolve().parents[3]
    evidence = _code_evidence(repo_root)
    paths = {row["path"] for row in evidence["files"]}
    for name in (
        "conversion_types", "source_common", "transform_common", "source", "transform",
        "mobiles", "objects", "rooms", "zones", "shops", "quests", "soc",
        "weapon_mapping", "weapon_table", "mobile_identity", "mob_calculator",
    ):
      self.assertIn(f"scripts/world/wtool_lib/rol_{name}.py", paths)
    for relative in (
        "scripts/world/wtool_lib/rol_weapon_overrides.json",
        "scripts/world/rol_conversion_policy.json",
        "scripts/world/rol_source_race_registry.json",
        "scripts/world/wtool_constants.json",
        "scripts/world/wtool_lib/objects.py",
        "src/structs.h",
        "src/combat/assign_wpn_armor.c",
        "src/magic/magic.c",
        "src/net/protocol.c",
        "unittests/CuTest/test_protocol_parser.c",
        "unittests/CuTest/test_race_equivalence.c",
        "unittests/CuTest/test_unassigned_spells.c",
        "bin/rol_mob_calculator",
    ):
      self.assertIn(relative, paths)
    self.assertEqual(len(paths), len(_CODE_EVIDENCE_PATHS))

  def test_completion_rejects_changed_captured_converter_inputs(self) -> None:
    with tempfile.TemporaryDirectory() as temporary, ExitStack() as stack:
      root = Path(temporary)
      bundle = root / "bundle"
      (bundle / "validation").mkdir(parents=True)
      (root / "lib").mkdir()
      (root / "lib/.env").write_text("APP_ENV=development\n", encoding="ascii")
      # Real evidence capture and completion hashing; isolate unrelated world,
      # persistence, and documentation gates already covered by their own suites.
      for relative in (*_CODE_EVIDENCE_PATHS, "bin/luminari"):
        path = root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(b"captured input\n")
      (bundle / "code-evidence.json").write_text(json.dumps(_code_evidence(root)))
      validation = {"complete": True, "summary": {}, "findings": []}
      (bundle / "validation/candidate.json").write_text(json.dumps(validation))
      replacements = {
          "default_repo_root": root,
          "_verify_bundle": {
              "acceptance": {"ready_to_apply": True},
              "candidate_tree_sha256": "candidate",
              "run_id": "release",
          },
          "tree_manifest": {"tree_sha256": "candidate"},
          "load_manifest": {},
          "resolve_config": None,
          "validate_indexed_world": None,
          "result_payload": validation,
          "apply_phase8_bundle": {"idempotent_no_op": True, "changed_paths": []},
          "_documentation_audit": {"pass": True},
      }
      for name, result in replacements.items():
        stack.enter_context(patch(f"wtool_lib.rol_phase8.{name}", return_value=result))
      unchanged = write_phase8_completion(bundle, root / "lib", root / "unchanged")
      self.assertTrue(unchanged["complete"])
      for index, relative in enumerate((
          "scripts/world/wtool_lib/rol_objects.py",
          "scripts/world/wtool_lib/rol_weapon_overrides.json",
          "src/net/protocol.c",
      )):
        with self.subTest(input=relative):
          path = root / relative
          path.write_bytes(b"changed after release gates\n")
          output = root / f"changed-{index}"
          changed = write_phase8_completion(bundle, root / "lib", output)
          self.assertFalse(changed["complete"])
          acceptance = json.loads((output / "acceptance.json").read_text())
          self.assertFalse(acceptance["runtime_code_unchanged_since_gates"])
          path.write_bytes(b"captured input\n")

  @staticmethod
  def _empty_world(*, zones=None):
    return SimpleNamespace(
        zones=[] if zones is None else zones,
        rooms=[],
        mobiles=[],
        objects=[],
        shops=[],
        hlquests=[],
    )

  def test_documentation_audit_uses_permanent_references(self) -> None:
    repo_root = Path(__file__).resolve().parents[3]

    audit = _documentation_audit(repo_root)

    self.assertTrue(audit["canonical_contract_present"])
    self.assertTrue(audit["isolation_correction_changelog_present"])
    self.assertTrue(audit["pass"])

  def test_action_audit_accepts_clean_canonical_targets(self) -> None:
    actions = [
        {
            "source_record_id": f"room:{index}",
            "source_kind": "wld",
            "destination_vnum": 2_000_000 + index,
            "final_action": "ADD",
        }
        for index in range(71_680)
    ]

    audit = _action_audit(actions, {"findings": []})

    self.assertTrue(audit["pass"])
    self.assertEqual(71_680, audit["rows"])
    self.assertEqual(71_680, audit["selected_target_records"])

  def test_action_audit_rejects_blocking_selected_record_error(self) -> None:
    actions = [
        {
            "source_record_id": f"room:{index}",
            "source_kind": "wld",
            "destination_vnum": 2_000_000 + index,
            "final_action": "ADD",
        }
        for index in range(71_680)
    ]
    validation = {
        "findings": [
            {
                "severity": "error",
                "suppressed": False,
                "record_type": "room",
                "vnum": 2_000_001,
            }
        ]
    }

    audit = _action_audit(actions, validation)

    self.assertFalse(audit["pass"])
    self.assertEqual(1, len(audit["blocking_selected_record_findings"]))

  def test_action_audit_accepts_canonical_merge_destination_already_in_target(self) -> None:
    actions = [
        {
            "source_record_id": f"room:{index}",
            "source_kind": "wld",
            "destination_vnum": 2_000_000 + index,
            "final_action": "ADD",
        }
        for index in range(71_679)
    ]
    actions.append(
        {
            "source_record_id": "zone:source-internal-merge",
            "source_kind": "zon",
            "destination_vnum": 20_000,
            "final_action": "MERGE",
        }
    )
    baseline = self._empty_world(zones=[SimpleNamespace(vnum=20_000)])

    audit = _action_audit(actions, {"findings": []}, baseline)

    self.assertTrue(audit["pass"])
    self.assertEqual(
        ["zone:source-internal-merge"],
        audit["source_internal_merges_with_existing_target_destination"],
    )

  def test_line_format_audit_requires_ascii_lf(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      root = Path(temporary)
      (root / "good").write_bytes(b"good\n")
      (root / "crlf").write_bytes(b"bad\r\n")
      (root / "nonascii").write_bytes(b"bad\xff\n")

      audit = _line_format_audit(root, ("good", "crlf", "nonascii"))

    self.assertFalse(audit["pass"])
    by_path = {row["path"]: row for row in audit["files"]}
    self.assertTrue(by_path["good"]["ascii"])
    self.assertFalse(by_path["crlf"]["lf_only"])
    self.assertFalse(by_path["nonascii"]["ascii"])

  def test_repeat_evidence_ignores_only_creation_time(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      root = Path(temporary)
      primary = root / "primary"
      repeat = root / "repeat"
      for bundle, creation_time in (
          (primary, "2026-08-17T00:00:00Z"),
          (repeat, "2026-08-17T00:01:00Z"),
      ):
        output = bundle / "output/world"
        output.mkdir(parents=True)
        (output / "candidate.dat").write_text("identical\n", encoding="ascii")
        (bundle / "run-manifest.json").write_text(
            json.dumps(
                {
                    "phase": 7,
                    "stage": "cumulative-milestone",
                    "run_id": "rol-phase7-identical",
                    "creation_time": creation_time,
                    "artifacts": [],
                    "acceptance": {"complete": True},
                }
            ),
            encoding="ascii",
        )

      evidence = _repeat_evidence(primary, repeat)

    self.assertTrue(evidence["manifest_content_identical"])
    self.assertTrue(evidence["output_tree_byte_identical"])
    self.assertTrue(evidence["pass"])

  def test_mechanics_isolation_requires_reserved_owners(self) -> None:
    repo_root = Path(__file__).resolve().parents[3]
    baseline = self._empty_world()
    candidate = self._empty_world(
        zones=[SimpleNamespace(vnum=20_000, flags=list(encode_bits({18})))]
    )

    audit = _mechanics_isolation_audit(repo_root, baseline, candidate)

    self.assertTrue(audit["pass"])
    self.assertEqual(0, audit["baseline_rol_markers"])
    self.assertEqual(0, audit["low_namespace_candidate_markers"])
    self.assertGreater(audit["seven_digit_identity_literals"], 0)

  def test_mechanics_isolation_rejects_low_namespace_marker(self) -> None:
    repo_root = Path(__file__).resolve().parents[3]
    baseline = self._empty_world()
    candidate = self._empty_world(
        zones=[SimpleNamespace(vnum=1_507, flags=list(encode_bits({18})))]
    )

    audit = _mechanics_isolation_audit(repo_root, baseline, candidate)

    self.assertFalse(audit["pass"])
    self.assertEqual(1, audit["low_namespace_candidate_markers"])

  def test_mechanics_isolation_accepts_reserved_marker_in_recovery_baseline(self) -> None:
    repo_root = Path(__file__).resolve().parents[3]
    baseline = self._empty_world(
        zones=[SimpleNamespace(vnum=20_000, flags=list(encode_bits({18})))]
    )
    candidate = self._empty_world(
        zones=[SimpleNamespace(vnum=20_000, flags=list(encode_bits({18})))]
    )

    audit = _mechanics_isolation_audit(repo_root, baseline, candidate)

    self.assertTrue(audit["pass"])
    self.assertEqual(1, audit["baseline_rol_markers"])
    self.assertEqual(0, audit["low_namespace_baseline_markers"])


if __name__ == "__main__":
  unittest.main()
