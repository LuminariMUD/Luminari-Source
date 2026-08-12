from __future__ import annotations

import json
from pathlib import Path
import tempfile
import unittest

from wtool_lib.constants import default_repo_root
from wtool_lib.rol_special_reconciliation import (
    handler_disposition,
    source_handler_definitions,
    write_special_reconciliation_bundle,
)


class RolSpecialReconciliationTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.root = default_repo_root()

  def test_reviewed_handler_dispositions_do_not_rely_on_matching_names(self) -> None:
    guild = handler_disposition("guild")
    dump = handler_disposition("dump")
    unknown = handler_disposition("not_reviewed")

    self.assertEqual("RoL Guild Room", guild["target"])
    self.assertEqual("NATIVE_PERSISTED", guild["strategy"])
    self.assertEqual("SOURCE_INERT_EXCLUDED", dump["strategy"])
    self.assertIn("returns before", dump["reason"])
    self.assertEqual("pending", unknown["status"])

  def test_source_definition_scanner_ignores_comment_and_string_decoys(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      source_root = Path(temporary)
      (source_root / "src").mkdir()
      (source_root / "src/sample.c").write_text(
          "/* int decoy(void) { } */\n"
          "const char *text = \"int string_decoy(void) { }\";\n"
          "int actual(void)\n"
          "{\n"
          "  return 1;\n"
          "}\n",
          encoding="ascii",
      )

      definitions = source_handler_definitions(
          source_root, {"decoy", "string_decoy", "actual"}
      )

    self.assertEqual(["actual"], sorted(definitions))
    self.assertEqual(3, definitions["actual"]["line"])
    self.assertEqual(4, definitions["actual"]["lines"])

  def test_production_inputs_generate_complete_progress_ledgers(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      summary = write_special_reconciliation_bundle(
          self.root / "lib/rol-conversion/runs/phase1-e6ea7982",
          self.root / "lib/rol-conversion/runs/phase2-e6ea7982",
          self.root / "lib/rol-conversion/runs/phase5-shop-20260812-audit",
          self.root / "EXAMPLE/RealmsOfLuminari",
          Path(temporary) / "phase6",
          created_at="2026-08-12T02:05:00Z",
      )

      self.assertEqual(1_234, summary["active_direct_bindings"])
      self.assertEqual(605, summary["source_handlers"])
      self.assertEqual(605, summary["source_handler_definitions_located"])
      self.assertEqual(185, summary["direct_bindings_by_status"]["resolved"])
      self.assertEqual(1_049, summary["direct_bindings_by_status"]["pending"])
      self.assertEqual(848, summary["act_spec_records"])
      self.assertEqual(455, summary["act_spec_by_status"]["resolved"])
      self.assertEqual(393, summary["act_spec_by_status"]["pending"])

      output_dir = Path(temporary) / "phase6"
      manifest = json.loads((output_dir / "run-manifest.json").read_text(encoding="ascii"))
      expected_records = {
          "act-spec-ledger.jsonl": 848,
          "binding-ledger.jsonl": 1_234,
          "handler-inventory.jsonl": 605,
      }
      for artifact in manifest["artifacts"]:
        if artifact["path"] in expected_records:
          lines = (output_dir / artifact["path"]).read_text(encoding="ascii").splitlines()
          self.assertEqual(expected_records[artifact["path"]], artifact["records"])
          self.assertEqual(expected_records[artifact["path"]], len(lines))


if __name__ == "__main__":
  unittest.main()
