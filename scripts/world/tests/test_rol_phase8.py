from __future__ import annotations

from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest

from wtool_lib.flags import encode_bits
from wtool_lib.rol_phase8 import (
    _action_audit,
    _documentation_audit,
    _line_format_audit,
    _mechanics_isolation_audit,
)


class RolPhase8Tests(unittest.TestCase):
  @staticmethod
  def _empty_world(*, zones=None):
    return SimpleNamespace(
        zones=[] if zones is None else zones,
        rooms=[],
        mobiles=[],
        objects=[],
        shops=[],
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


if __name__ == "__main__":
  unittest.main()
