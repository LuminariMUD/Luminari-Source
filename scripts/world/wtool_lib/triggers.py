"""Parser for DG Script trigger prototype files."""

from __future__ import annotations

from pathlib import Path
import re
from typing import Any

from .flags import decode_tokens
from .models import SourceSpan, TriggerRecord
from .parsing import ParseResult, finding, source_issue_finding
from .source import SourceCursor, SourceFile, parse_c_integer_token


_TRIGGER_LINE_LIMIT = 255


def _line_limit(result: ParseResult[TriggerRecord], line: Any, vnum: int | None = None) -> None:
  if line.byte_length <= _TRIGGER_LINE_LIMIT:
    return
  result.findings.append(
      finding(
          "TRG001",
          "error",
          f"physical line is {line.byte_length} bytes; the 256-byte trigger buffer safely holds "
          f"at most {_TRIGGER_LINE_LIMIT}",
          line.span,
          "trigger" if vnum is not None else None,
          vnum,
      )
  )
  result.complete = False


def _read_string(
    cursor: SourceCursor,
    result: ParseResult[TriggerRecord],
    vnum: int,
) -> tuple[str, bool]:
  value = cursor.read_tilde_string()
  for issue in value.issues:
    result.findings.append(source_issue_finding(issue, "TRG", "trigger", vnum))
    if issue.fatal:
      result.complete = False
  return value.text, value.terminated


def _recover_record(cursor: SourceCursor) -> None:
  while not cursor.eof:
    line = cursor.peek_raw()
    if line is not None and (line.text.startswith("#") or line.text.startswith("$")):
      return
    cursor.read_raw()


def parse_trigger_file(
    path: Path,
    display_path: str,
    manifest: dict[str, Any],
) -> ParseResult[TriggerRecord]:
  result: ParseResult[TriggerRecord] = ParseResult()
  try:
    source = SourceFile.from_path(path, display_path)
  except OSError as error:
    result.findings.append(
        finding("TRG002", "error", f"cannot read trigger file: {error}", SourceSpan(display_path, 1))
    )
    result.complete = False
    return result
  cursor = SourceCursor(source)
  found_end = False
  while True:
    header = cursor.read_significant()
    if header is None:
      break
    _line_limit(result, header)
    if header.text.startswith("$"):
      found_end = True
      break
    match = re.match(r"^#([+-]?\d+)", header.text)
    if match is None:
      result.findings.append(finding("TRG003", "error", "expected a #vnum trigger header", header.span))
      _recover_record(cursor)
      continue
    parsed_vnum = parse_c_integer_token(match.group(1))
    if parsed_vnum.error is not None or parsed_vnum.value is None:
      result.findings.append(finding("TRG003", "error", f"invalid trigger vnum: {parsed_vnum.error}", header.span))
      _recover_record(cursor)
      continue
    record = TriggerRecord(parsed_vnum.value, header.span, path.stem)
    result.records.append(record)
    if record.vnum < 0:
      result.findings.append(
          finding("TRG004", "error", "trigger vnum must be non-negative", header.span, "trigger", record.vnum)
      )

    record.name, complete = _read_string(cursor, result, record.vnum)
    if not complete:
      record.complete = False
      return result
    numeric = cursor.read_significant()
    if numeric is None:
      result.findings.append(
          finding("TRG005", "error", "trigger ends before its numeric header", header.span, "trigger", record.vnum)
      )
      result.complete = False
      record.complete = False
      return result
    _line_limit(result, numeric, record.vnum)
    tokens = numeric.text.split()
    if len(tokens) < 2:
      result.findings.append(
          finding(
              "TRG005",
              "error",
              "trigger numeric header requires attach type and flags",
              numeric.span,
              "trigger",
              record.vnum,
          )
      )
      _recover_record(cursor)
      continue
    attach = parse_c_integer_token(tokens[0])
    if attach.error is not None or attach.value is None:
      result.findings.append(
          finding("TRG005", "error", "trigger attach type is not an integer", numeric.span, "trigger", record.vnum)
      )
      _recover_record(cursor)
      continue
    record.attach_type = attach.value
    record.type_flags = tokens[1]
    if len(tokens) >= 3:
      narg = parse_c_integer_token(tokens[2])
      if narg.error is not None or narg.value is None:
        result.findings.append(
            finding(
                "TRG006",
                "error",
                "trigger numeric argument is not an integer",
                numeric.span,
                "trigger",
                record.vnum,
            )
        )
      else:
        record.numeric_argument = narg.value
    else:
      record.numeric_argument = 0
    if record.attach_type not in {0, 1, 2}:
      result.findings.append(
          finding(
              "TRG007",
              "error",
              f"trigger attach type {record.attach_type} is outside 0..2",
              numeric.span,
              "trigger",
              record.vnum,
          )
      )
    else:
      table_key = {
          0: "trigger-types-mob",
          1: "trigger-types-object",
          2: "trigger-types-world",
      }[record.attach_type]
      table = manifest["tables"][table_key]
      decoded = decode_tokens([record.type_flags], len(table["entries"]))
      record.type_bits = set(decoded.bits)
      for issue in decoded.issues:
        result.findings.append(
            finding(
                "TRG008",
                "error",
                f"invalid flags for attach type {record.attach_type}: {issue.message}",
                numeric.span,
                "trigger",
                record.vnum,
            )
        )

    record.argument_list, complete = _read_string(cursor, result, record.vnum)
    if not complete:
      record.complete = False
      return result
    record.commands, complete = _read_string(cursor, result, record.vnum)
    if not complete:
      record.complete = False
      return result
    if not record.commands:
      result.findings.append(
          finding(
              "TRG009",
              "error",
              "empty trigger command body makes parse_trigger() pass NULL to strdup()",
              header.span,
              "trigger",
              record.vnum,
          )
      )

  if not found_end:
    result.findings.append(
        finding("TRG010", "error", "trigger file is missing its '$' terminator", SourceSpan(display_path, 1))
    )
    result.complete = False
  return result
