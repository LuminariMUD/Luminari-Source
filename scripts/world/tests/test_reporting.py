from __future__ import annotations

import json
import unittest

from wtool_lib.models import (
    Finding,
    JSON_SCHEMA_VERSION,
    SourceSpan,
    TOOL_VERSION,
    ValidationResult,
)
from wtool_lib.reporting import exit_status, render_human, render_json


class ReportingTests(unittest.TestCase):
  def result(self) -> ValidationResult:
    return ValidationResult(
        "fixture-world",
        "all",
        findings=[
            Finding("SEM002", "warning", "second", SourceSpan("z.wld", 4)),
            Finding("IDX001", "error", "first", SourceSpan("a/index", 2)),
            Finding("SEM001", "info", "suppressed", SourceSpan("z.wld", 3)),
        ],
        config={"diagonal_dirs": False},
    )

  def test_json_is_deterministic_and_sorted(self) -> None:
    first = render_json(self.result(), {"SEM001"})
    second = render_json(self.result(), {"SEM001"})
    self.assertEqual(first, second)
    payload = json.loads(first)
    self.assertEqual(["IDX001", "SEM001", "SEM002"], [item["code"] for item in payload["findings"]])
    self.assertTrue(payload["findings"][1]["suppressed"])
    self.assertNotIn("timestamp", payload)

  def test_additive_record_types_bump_tool_but_not_json_schema(self) -> None:
    self.assertEqual("0.6.0", TOOL_VERSION)
    self.assertEqual(1, JSON_SCHEMA_VERSION)
    payload = json.loads(render_json(self.result()))
    self.assertEqual("0.6.0", payload["tool_version"])
    self.assertEqual(1, payload["schema_version"])

  def test_human_output_omits_suppressed_findings(self) -> None:
    output = render_human(self.result(), {"SEM001"})
    self.assertNotIn("SEM001", output)
    self.assertIn("1 suppressed", output)

  def test_exit_thresholds(self) -> None:
    self.assertEqual(1, exit_status(self.result()))
    warning = ValidationResult(
        "fixture", "all", [Finding("SEM002", "warning", "warning", SourceSpan("x", 1))]
    )
    self.assertEqual(0, exit_status(warning))
    self.assertEqual(1, exit_status(warning, strict=True))
    self.assertEqual(0, exit_status(warning, strict=True, ignored_codes={"SEM002"}))


if __name__ == "__main__":
  unittest.main()
