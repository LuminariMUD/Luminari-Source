"""Deterministic human and JSON reporting for wtool."""

from __future__ import annotations

from collections import Counter
import json
from typing import Any

from .models import Finding, JSON_SCHEMA_VERSION, TOOL_VERSION, ValidationResult


SEVERITIES = ("error", "warning", "info")


def _count_findings(findings: list[Finding], suppressed: bool) -> dict[str, Any]:
  selected = [finding for finding in findings if finding.suppressed is suppressed]
  by_severity = Counter(finding.severity for finding in selected)
  by_code = Counter(finding.code for finding in selected)
  return {
      "by_severity": {severity: by_severity.get(severity, 0) for severity in SEVERITIES},
      "by_code": dict(sorted(by_code.items())),
      "total": len(selected),
  }


def result_payload(result: ValidationResult, ignored_codes: set[str] | None = None) -> dict[str, Any]:
  findings = result.normalized_findings(ignored_codes)
  return {
      "schema_version": JSON_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "root": result.root_label,
      "mode": result.mode,
      "config": result.config,
      "complete": result.complete,
      "findings": [finding.to_dict() for finding in findings],
      "summary": {
          "active": _count_findings(findings, suppressed=False),
          "suppressed": _count_findings(findings, suppressed=True),
      },
  }


def render_json(result: ValidationResult, ignored_codes: set[str] | None = None) -> str:
  return json.dumps(
      result_payload(result, ignored_codes),
      ensure_ascii=True,
      indent=2,
      sort_keys=True,
  ) + "\n"


def render_human(result: ValidationResult, ignored_codes: set[str] | None = None) -> str:
  findings = result.normalized_findings(ignored_codes)
  lines: list[str] = []
  for finding in findings:
    if finding.suppressed:
      continue
    location = f"{finding.span.path}:{finding.span.line}"
    if finding.span.column != 1:
      location += f":{finding.span.column}"
    record = ""
    if finding.record_type is not None and finding.vnum is not None:
      record = f" [{finding.record_type} {finding.vnum}]"
    lines.append(
        f"{location}: {finding.severity} {finding.code}: {finding.message}{record}"
    )

  active = _count_findings(findings, suppressed=False)
  suppressed = _count_findings(findings, suppressed=True)
  counts = active["by_severity"]
  status = "complete" if result.complete else "incomplete"
  lines.append(
      f"Summary: {counts['error']} error(s), {counts['warning']} warning(s), "
      f"{counts['info']} info finding(s); {suppressed['total']} suppressed; parse {status}."
  )
  return "\n".join(lines) + "\n"


def exit_status(
    result: ValidationResult,
    strict: bool = False,
    ignored_codes: set[str] | None = None,
) -> int:
  findings = result.normalized_findings(ignored_codes)
  active = [finding for finding in findings if not finding.suppressed]
  if any(finding.severity == "error" for finding in active):
    return 1
  if strict and any(finding.severity == "warning" for finding in active):
    return 1
  return 0
