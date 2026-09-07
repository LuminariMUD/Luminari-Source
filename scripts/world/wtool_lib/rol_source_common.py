"""Shared source segmentation, rows, strings, and diagnostics."""

from __future__ import annotations

import hashlib
import re

from .rol_conversion_types import RolDiagnostic, RolRecord, RolReference, RolSourceCorpus
from .source import SourceFile, SourceLine


_HEADER = re.compile(br"#(\d+)\s*$")


_INTEGER = re.compile(rb"[+-]?\d+")


def _line_bytes(lines: list[SourceLine], start: int, end: int) -> bytes:
  return b"".join(line.raw + line.newline for line in lines[start:end])


def _segment_hash(lines: list[SourceLine], start: int, end: int) -> str:
  return hashlib.sha256(_line_bytes(lines, start, end)).hexdigest()


def _segments(source: SourceFile) -> list[tuple[int, int, int]]:
  headers: list[tuple[int, int]] = []
  for index, line in enumerate(source.lines):
    match = _HEADER.fullmatch(line.raw)
    if match is not None:
      headers.append((index, int(match.group(1))))
  return [
      (start, headers[index + 1][0] if index + 1 < len(headers) else len(source.lines), vnum)
      for index, (start, vnum) in enumerate(headers)
  ]


def _next_content(lines: list[SourceLine], position: int, end: int) -> tuple[int, SourceLine | None]:
  while position < end:
    line = lines[position]
    position += 1
    stripped = line.raw.strip()
    if not stripped or stripped.startswith(b"*") or stripped.startswith(b";;"):
      continue
    return position, line
  return position, None


def _read_tilde(
    lines: list[SourceLine],
    position: int,
    end: int,
) -> tuple[int, str | None, bool]:
  pieces: list[bytes] = []
  while position < end:
    line = lines[position]
    position += 1
    stripped = line.raw.rstrip()
    if stripped.endswith(b"~"):
      pieces.append(stripped[:-1])
      return position, b"\r\n".join(pieces).decode("utf-8", errors="surrogateescape"), True
    pieces.append(line.raw)
  value = b"\r\n".join(pieces).decode("utf-8", errors="surrogateescape")
  return position, value or None, False


def _integers(line: SourceLine) -> list[int]:
  return [int(token) for token in _INTEGER.findall(line.raw)]


def _numeric_line(line: SourceLine) -> bool:
  return re.fullmatch(br"[+-]?\d+(?:\s+[+-]?\d+)*", line.raw.strip()) is not None


def _collect_numeric_lines(
    lines: list[SourceLine],
    position: int,
    end: int,
    values: list[int],
    minimum: int,
) -> tuple[int, list[int], SourceLine | None]:
  """Collect scanf-style integer arguments without consuming the next directive."""

  last_line: SourceLine | None = None
  while len(values) < minimum and position < end:
    probe = position
    next_position, line = _next_content(lines, probe, end)
    if line is None or not _numeric_line(line):
      break
    position = next_position
    values.extend(_integers(line))
    last_line = line
  return position, values, last_line


def _diagnostic(
    corpus: RolSourceCorpus,
    code: str,
    severity: str,
    message: str,
    line: SourceLine,
    kind: str | None = None,
    vnum: int | None = None,
) -> None:
  corpus.diagnostics.append(
      RolDiagnostic(code, severity, message, line.display_path, line.number, kind, vnum)
  )


def _exclude_record(
    corpus: RolSourceCorpus,
    record: RolRecord,
    code: str,
    message: str,
    line: SourceLine,
) -> None:
  """Account for a source record that cannot be loaded without inventing intent."""

  record.complete = True
  record.values["source_disposition"] = "EXCLUDE"
  record.values["source_exclusion_reason"] = message
  record.directives.append({"token": "EXCLUDED_SOURCE_RECORD", "line": line.number})
  _diagnostic(corpus, code, "warning", message, line, record.kind, record.vnum)


def _reference(
    record: RolRecord,
    target_type: str,
    target_vnum: int,
    role: str,
    line: SourceLine,
    allow_negative: bool = False,
) -> None:
  if target_vnum < 0 and not allow_negative:
    return
  record.references.append(
      RolReference(target_type, target_vnum, role, line.display_path, line.number)
  )


def _new_record(
    source: SourceFile,
    basename: str,
    kind: str,
    start: int,
    end: int,
    vnum: int,
) -> RolRecord:
  return RolRecord(
      kind=kind,
      vnum=vnum,
      basename=basename,
      path=source.display_path,
      line=source.lines[start].number,
      end_line=source.lines[end - 1].number if end > start else source.lines[start].number,
      sha256=_segment_hash(source.lines, start, end),
  )
