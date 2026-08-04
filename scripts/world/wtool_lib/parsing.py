"""Shared parser result and numeric scanning helpers."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Generic, TypeVar

from .models import Finding, SourceSpan
from .source import SourceIssue, parse_c_integer_prefix


RecordT = TypeVar("RecordT")


@dataclass(slots=True)
class ParseResult(Generic[RecordT]):
  records: list[RecordT] = field(default_factory=list)
  findings: list[Finding] = field(default_factory=list)
  complete: bool = True


def scan_integers(text: str, maximum: int) -> tuple[list[int], int, str | None]:
  """Scan up to maximum whitespace-separated scanf-style decimal conversions."""

  values: list[int] = []
  offset = 0
  for _ in range(maximum):
    parsed = parse_c_integer_prefix(text[offset:])
    if parsed.consumed == 0:
      break
    if parsed.error is not None:
      return values, offset + parsed.consumed, parsed.error
    assert parsed.value is not None
    values.append(parsed.value)
    offset += parsed.consumed
    if offset < len(text) and not text[offset].isspace():
      break
  return values, offset, None


def strip_optional_tilde(text: str) -> str:
  position = text.find("~")
  return text if position < 0 else text[:position]


def source_issue_finding(
    issue: SourceIssue,
    prefix: str,
    record_type: str | None = None,
    vnum: int | None = None,
) -> Finding:
  suffix = {"SRC001": "001", "SRC002": "002", "SRC003": "003"}.get(issue.code, "099")
  return Finding(
      f"{prefix}{suffix}",
      "error",
      issue.message,
      issue.span,
      record_type=record_type,
      vnum=vnum,
  )


def finding(
    code: str,
    severity: str,
    message: str,
    span: SourceSpan,
    record_type: str | None = None,
    vnum: int | None = None,
) -> Finding:
  return Finding(code, severity, message, span, record_type=record_type, vnum=vnum)
