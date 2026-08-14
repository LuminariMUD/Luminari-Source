from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from wtool_lib.rol_phase8 import _action_audit, _line_format_audit


class RolPhase8Tests(unittest.TestCase):
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


if __name__ == "__main__":
  unittest.main()
