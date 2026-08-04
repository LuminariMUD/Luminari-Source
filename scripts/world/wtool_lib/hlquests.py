"""Byte-safe parser for Luminari high-level quest flat files."""

from __future__ import annotations

from pathlib import Path
import re
from typing import Any

from .models import (
    HlQuestCommandRecord,
    HlQuestEntryRecord,
    HlQuestRecord,
    SourceSpan,
    VnumReference,
)
from .parsing import ParseResult, finding, source_issue_finding
from .source import (
    READ_SIZE,
    SourceCursor,
    SourceFile,
    parse_c_integer_prefix,
)


_HLQ_LINE_BUFFER = 256
_CREDIBLE_HEADER = re.compile(r"^#[+-]?\d+[ \t\v\f]*$")
_CREDIBLE_ENTRY = re.compile(r"^[AQR]!?$")
_TOKEN = re.compile(r"[ \t\v\f]*(\S+)")
_INTEGER = re.compile(r"[ \t\v\f]*([+-]?\d+)")


def _span(line: Any, start: int, end: int) -> SourceSpan:
  return SourceSpan(
      line.span.path,
      line.span.line,
      start + 1,
      end_line=line.span.line,
      end_column=end + 1,
  )


def _line_limit(
    result: ParseResult[HlQuestRecord],
    line: Any,
    record: HlQuestRecord | None = None,
    entry: HlQuestEntryRecord | None = None,
) -> None:
  if line.byte_length <= _HLQ_LINE_BUFFER - 1:
    return
  result.findings.append(
      finding(
          "HLQ001",
          "error",
          f"physical line is {line.byte_length} bytes; boot_the_quests() passes a "
          f"{_HLQ_LINE_BUFFER}-byte buffer to get_line(), so at most "
          f"{_HLQ_LINE_BUFFER - 1} bytes are safe",
          line.span,
          "hlquest" if record is not None else None,
          record.vnum if record is not None else None,
      )
  )
  result.complete = False
  if record is not None:
    record.complete = False
  if entry is not None:
    entry.complete = False


def _mark_incomplete(
    result: ParseResult[HlQuestRecord],
    record: HlQuestRecord | None = None,
    entry: HlQuestEntryRecord | None = None,
    command: HlQuestCommandRecord | None = None,
) -> None:
  result.complete = False
  if record is not None:
    record.complete = False
  if entry is not None:
    entry.complete = False
  if command is not None:
    command.complete = False


def _record_error(
    result: ParseResult[HlQuestRecord],
    record: HlQuestRecord,
    code: str,
    message: str,
    span: SourceSpan,
    entry: HlQuestEntryRecord | None = None,
) -> None:
  result.findings.append(
      finding(code, "error", message, span, "hlquest", record.vnum)
  )
  _mark_incomplete(result, record, entry)


def _is_recovery_boundary(line: Any) -> bool:
  return bool(
      _CREDIBLE_HEADER.fullmatch(line.text)
      or _CREDIBLE_ENTRY.fullmatch(line.text)
      or line.raw.startswith(b"$")
  )


def _read_string(
    cursor: SourceCursor,
    result: ParseResult[HlQuestRecord],
    record: HlQuestRecord,
    entry: HlQuestEntryRecord,
    field_name: str,
) -> bool:
  value = cursor.read_tilde_string()
  for issue in value.issues:
    result.findings.append(
        source_issue_finding(issue, "HLQ", "hlquest", record.vnum)
    )
    if issue.fatal:
      _mark_incomplete(result, record, entry)
  setattr(entry, field_name, value.text)
  entry.field_spans[field_name] = value.span
  return value.terminated


def _discard_string(
    cursor: SourceCursor,
    result: ParseResult[HlQuestRecord],
) -> bool:
  value = cursor.read_tilde_string()
  for issue in value.issues:
    result.findings.append(source_issue_finding(issue, "HLQ"))
    if issue.fatal:
      result.complete = False
  return value.terminated


def _skip_orphan_entry(
    cursor: SourceCursor,
    result: ParseResult[HlQuestRecord],
    marker: Any,
) -> None:
  kind = marker.text[0]
  if kind == "R":
    room_line = cursor.read_significant()
    if room_line is None:
      return
    _line_limit(result, room_line)
    if _is_recovery_boundary(room_line):
      cursor.unread_raw()
      return
  string_count = 2 if kind == "A" else 1
  for _ in range(string_count):
    if not _discard_string(cursor, result):
      return
  if kind == "A":
    return
  while True:
    line = cursor.read_significant()
    if line is None:
      return
    _line_limit(result, line)
    if line.raw.startswith(b"S"):
      return
    if _is_recovery_boundary(line):
      cursor.unread_raw()
      return


def _entry_type_map(manifest: dict[str, Any]) -> dict[str, int]:
  by_macro = {
      entry["macro"]: entry["index"]
      for entry in manifest["tables"]["hlquest-entry-types"]["entries"]
  }
  return {
      "A": by_macro["QUEST_ASK"],
      "Q": by_macro["QUEST_GIVE"],
      "R": by_macro["QUEST_ROOM"],
  }


def _command_type_map(manifest: dict[str, Any]) -> dict[str, int]:
  return {
      entry["code"]: entry["index"]
      for entry in manifest["tables"]["hlquest-commands"]["entries"]
  }


def _parse_room(
    cursor: SourceCursor,
    result: ParseResult[HlQuestRecord],
    record: HlQuestRecord,
    entry: HlQuestEntryRecord,
) -> bool:
  line = cursor.read_significant()
  if line is None:
    _record_error(
        result,
        record,
        "HLQ009",
        f"host {record.vnum} entry {entry.physical_ordinal} ends before its ROOM number",
        entry.span,
        entry,
    )
    return False
  _line_limit(result, line, record, entry)
  if _is_recovery_boundary(line):
    cursor.unread_raw()
    _record_error(
        result,
        record,
        "HLQ009",
        f"host {record.vnum} entry {entry.physical_ordinal} is missing its ROOM number; "
        "the loader would reuse stale numeric state",
        entry.span,
        entry,
    )
    return False

  match = _INTEGER.match(line.text)
  parsed = parse_c_integer_prefix(line.text)
  if match is None or parsed.error is not None or parsed.value is None:
    detail = parsed.error or "expected a decimal integer"
    _record_error(
        result,
        record,
        "HLQ009",
        f"ROOM number is invalid and would reuse stale loader state: {detail}",
        line.span,
        entry,
    )
    entry.field_spans["room_vnum"] = line.span
    return True

  entry.room_vnum = parsed.value
  entry.field_spans["room_vnum"] = _span(line, match.start(1), match.end(1))
  if line.text[parsed.consumed :].strip():
    result.findings.append(
        finding(
            "HLQ009",
            "error",
            "ROOM number contains trailing data that sscanf() silently ignores",
            _span(line, parsed.consumed, len(line.text)),
            "hlquest",
            record.vnum,
        )
    )
  return True


def _scan_command(
    line: Any,
) -> tuple[str | None, SourceSpan | None, list[int], list[SourceSpan], int, str | None]:
  offset = 1
  token_match = _TOKEN.match(line.text, offset)
  if token_match is None:
    return None, None, [], [], offset, "expected a command code"
  code_token = token_match.group(1)
  code_span = _span(line, token_match.start(1), token_match.end(1))
  offset = token_match.end(1)
  values: list[int] = []
  spans: list[SourceSpan] = []
  for _ in range(2):
    integer_match = _INTEGER.match(line.text, offset)
    parsed = parse_c_integer_prefix(line.text[offset:])
    if integer_match is None or parsed.error is not None or parsed.value is None:
      return code_token, code_span, values, spans, offset + parsed.consumed, parsed.error
    values.append(parsed.value)
    spans.append(
        _span(
            line,
            offset + integer_match.start(1),
            offset + integer_match.end(1),
        )
    )
    offset += parsed.consumed
  return code_token, code_span, values, spans, offset, None


def _parse_command(
    line: Any,
    result: ParseResult[HlQuestRecord],
    record: HlQuestRecord,
    entry: HlQuestEntryRecord,
    command_types: dict[str, int],
) -> None:
  direction_marker = line.text[:1]
  direction = {"I": "input", "O": "output"}.get(direction_marker)
  code_token, code_span, values, value_spans, consumed, scan_error = _scan_command(line)
  code = code_token[:1] if code_token else None
  command = HlQuestCommandRecord(
      direction_marker=direction_marker,
      direction=direction,
      code_token=code_token,
      code=code,
      command_type=command_types.get(code) if code is not None else None,
      value=values[0] if values else None,
      location=values[1] if len(values) > 1 else None,
      span=line.span,
      physical_ordinal=len(entry.commands) + 1,
      effective=direction is not None,
  )
  command.field_spans["direction"] = _span(line, 0, 1)
  if code_span is not None:
    command.field_spans["code"] = code_span
  if value_spans:
    command.field_spans["value"] = value_spans[0]
  if len(value_spans) > 1:
    command.field_spans["location"] = value_spans[1]
  entry.commands.append(command)

  if scan_error is not None or code_token is None or len(values) != 2:
    detail = f": {scan_error}" if scan_error else ""
    _record_error(
        result,
        record,
        "HLQ010",
        f"host {record.vnum} entry {entry.physical_ordinal} command "
        f"{command.physical_ordinal} has {1 + len(values) if code_token else 0} "
        f"conversions; expected code, value, and location{detail}",
        line.span,
        entry,
    )
    command.complete = False
  elif line.text[consumed:].strip():
    result.findings.append(
        finding(
            "HLQ010",
            "error",
            "command contains trailing data that sscanf() silently ignores",
            _span(line, consumed, len(line.text)),
            "hlquest",
            record.vnum,
        )
    )

  if code_token is not None and len(code_token) != 1:
    result.findings.append(
        finding(
            "HLQ011",
            "warning",
            f"command code token {code_token!r} is noncanonical; the loader uses only "
            f"its first character {code!r}",
            code_span or line.span,
            "hlquest",
            record.vnum,
        )
    )
  if code is not None and code not in command_types:
    _record_error(
        result,
        record,
        "HLQ011",
        f"unknown command code {code!r} leaves the loader command type uninitialized",
        code_span or line.span,
        entry,
    )
    command.complete = False
  if direction is None:
    result.findings.append(
        finding(
            "HLQ012",
            "error",
            f"invalid command direction {direction_marker!r}; the loader discards this command",
            command.field_spans["direction"],
            "hlquest",
            record.vnum,
        )
    )


def _finalize_command_order(entry: HlQuestEntryRecord) -> None:
  inputs = [
      command
      for command in entry.commands
      if command.effective and command.direction == "input"
  ]
  outputs = [
      command
      for command in entry.commands
      if command.effective and command.direction == "output"
  ]
  for ordinal, command in enumerate(reversed(inputs), start=1):
    command.effective_runtime_ordinal = ordinal
  for ordinal, command in enumerate(outputs, start=1):
    command.effective_runtime_ordinal = ordinal


def _parse_chain(
    cursor: SourceCursor,
    result: ParseResult[HlQuestRecord],
    record: HlQuestRecord,
    entry: HlQuestEntryRecord,
    command_types: dict[str, int],
) -> bool:
  while True:
    line = cursor.read_significant()
    if line is None:
      _record_error(
          result,
          record,
          "HLQ013",
          f"host {record.vnum} entry {entry.physical_ordinal} command chain reaches "
          "end of file before S",
          entry.span,
          entry,
      )
      _finalize_command_order(entry)
      return False
    _line_limit(result, line, record, entry)
    if line.raw.startswith(b"S"):
      if line.text != "S":
        result.findings.append(
            finding(
                "HLQ013",
                "error",
                f"noncanonical chain terminator {line.text!r}; the loader treats any "
                "line beginning with 'S' as the terminator",
                line.span,
                "hlquest",
                record.vnum,
            )
        )
      _finalize_command_order(entry)
      return True
    if _is_recovery_boundary(line):
      cursor.unread_raw()
      _record_error(
          result,
          record,
          "HLQ013",
          f"host {record.vnum} entry {entry.physical_ordinal} command chain is missing S",
          entry.span,
          entry,
      )
      _finalize_command_order(entry)
      return False
    _parse_command(line, result, record, entry, command_types)


def _parse_entry(
    cursor: SourceCursor,
    result: ParseResult[HlQuestRecord],
    record: HlQuestRecord,
    marker: Any,
    entry_types: dict[str, int],
    command_types: dict[str, int],
) -> None:
  kind = marker.text[0]
  suffix = marker.text[1:]
  entry = HlQuestEntryRecord(
      entry_type=entry_types[kind],
      marker=kind,
      approval_suffix=suffix,
      approved=len(marker.text) > 1,
      span=marker.span,
      physical_ordinal=len(record.entries) + 1,
  )
  entry.field_spans["marker"] = _span(marker, 0, 1)
  if suffix:
    entry.field_spans["approval_suffix"] = _span(marker, 1, len(marker.text))
  record.entries.append(entry)

  if suffix not in {"", "!"}:
    result.findings.append(
        finding(
            "HLQ008",
            "warning",
            f"noncanonical approval suffix {suffix!r}; the loader treats any suffix "
            "as approved",
            entry.field_spans.get("approval_suffix", marker.span),
            "hlquest",
            record.vnum,
        )
    )

  if kind == "A":
    if not _read_string(cursor, result, record, entry, "keywords"):
      return
    _read_string(cursor, result, record, entry, "reply_message")
    return

  if kind == "R" and not _parse_room(cursor, result, record, entry):
    return
  if not _read_string(cursor, result, record, entry, "reply_message"):
    return
  entry.chain_terminated = _parse_chain(cursor, result, record, entry, command_types)


def _finalize_entry_order(record: HlQuestRecord) -> None:
  for ordinal, entry in enumerate(reversed(record.entries), start=1):
    entry.effective_runtime_ordinal = ordinal


def _reference_role(entry: HlQuestEntryRecord, command: HlQuestCommandRecord, role: str) -> str:
  direction = command.direction or command.direction_marker
  return (
      f"entry {entry.physical_ordinal} {direction} command "
      f"{command.physical_ordinal} {role}"
  )


def _add_reference(
    record: HlQuestRecord,
    target_type: str,
    target_vnum: int | None,
    role: str,
    span: SourceSpan,
) -> None:
  if target_vnum is None:
    return
  record.references.append(VnumReference(target_type, target_vnum, role, span))


def _populate_references(record: HlQuestRecord, manifest: dict[str, Any]) -> None:
  command_types = {
      entry["macro"]: entry["index"]
      for entry in manifest["tables"]["hlquest-commands"]["entries"]
  }
  entry_types = {
      entry["macro"]: entry["index"]
      for entry in manifest["tables"]["hlquest-entry-types"]["entries"]
  }
  record.references.clear()
  _add_reference(
      record,
      "mobile",
      record.vnum,
      "attached host mobile",
      record.field_spans.get("host_mobile_vnum", record.span),
  )
  for entry in record.entries:
    if entry.entry_type == entry_types["QUEST_ROOM"]:
      _add_reference(
          record,
          "room",
          entry.room_vnum,
          f"entry {entry.physical_ordinal} ROOM location",
          entry.field_spans.get("room_vnum", entry.span),
      )
    for command in entry.commands:
      if not command.effective or command.command_type is None:
        continue
      value_span = command.field_spans.get("value", command.span)
      location_span = command.field_spans.get("location", command.span)
      if command.command_type == command_types["QUEST_COMMAND_ITEM"]:
        _add_reference(
            record,
            "object",
            command.value,
            _reference_role(entry, command, "item"),
            value_span,
        )
      elif command.command_type == command_types["QUEST_COMMAND_LOAD_OBJECT_INROOM"]:
        _add_reference(
            record,
            "object",
            command.value,
            _reference_role(entry, command, "load-object prototype"),
            value_span,
        )
        if command.location != 0:
          _add_reference(
              record,
              "room",
              command.location,
              _reference_role(entry, command, "load destination"),
              location_span,
          )
      elif command.command_type == command_types["QUEST_COMMAND_LOAD_MOB_INROOM"]:
        _add_reference(
            record,
            "mobile",
            command.value,
            _reference_role(entry, command, "load-mobile prototype"),
            value_span,
        )
        if command.location != 0:
          _add_reference(
              record,
              "room",
              command.location,
              _reference_role(entry, command, "load destination"),
              location_span,
          )
      elif command.command_type == command_types["QUEST_COMMAND_OPEN_DOOR"]:
        _add_reference(
            record,
            "room",
            command.location,
            _reference_role(entry, command, "open-door location"),
            location_span,
        )


def _parse_host_header(
    line: Any,
    path: Path,
    result: ParseResult[HlQuestRecord],
) -> HlQuestRecord | None:
  integer_match = _INTEGER.match(line.text, 1)
  parsed = parse_c_integer_prefix(line.text[1:])
  if integer_match is None or parsed.error is not None or parsed.value is None:
    detail = parsed.error or "expected a decimal integer"
    result.findings.append(
        finding("HLQ005", "error", f"invalid host mobile header: {detail}", line.span)
    )
    result.complete = False
    return None

  record = HlQuestRecord(parsed.value, line.span, path.stem)
  record.field_spans["host_mobile_vnum"] = _span(
      line,
      1 + integer_match.start(1),
      1 + integer_match.end(1),
  )
  if line.text[1 + parsed.consumed :].strip():
    result.findings.append(
        finding(
            "HLQ005",
            "error",
            "host header contains trailing data that sscanf() silently ignores",
            _span(line, 1 + parsed.consumed, len(line.text)),
            "hlquest",
            record.vnum,
        )
    )
  if record.vnum < 0:
    result.findings.append(
        finding(
            "HLQ006",
            "error",
            "host mobile VNUM must be non-negative; real_mobile() cannot resolve it safely",
            record.field_spans["host_mobile_vnum"],
            "hlquest",
            record.vnum,
        )
    )
  result.records.append(record)
  return record


def parse_hlquest_file(
    path: Path,
    display_path: str,
    manifest: dict[str, Any],
) -> ParseResult[HlQuestRecord]:
  result: ParseResult[HlQuestRecord] = ParseResult()
  try:
    source = SourceFile.from_path(path, display_path)
  except OSError as error:
    result.findings.append(
        finding(
            "HLQ004",
            "error",
            f"cannot read high-level quest file: {error}",
            SourceSpan(display_path, 1),
        )
    )
    result.complete = False
    return result

  for source_line in source.lines:
    if source_line.raw.startswith(b"*") and source_line.byte_length > READ_SIZE - 2:
      result.findings.append(
          finding(
              "HLQ001",
              "error",
              f"comment line is {source_line.byte_length} bytes; get_line() reads at most "
              f"{READ_SIZE - 2} bytes before the remainder can enter the parser",
              source_line.span,
          )
      )
      result.complete = False

  cursor = SourceCursor(source)
  entry_types = _entry_type_map(manifest)
  command_types = _command_type_map(manifest)
  current: HlQuestRecord | None = None
  found_file_end = False
  truncated = False

  while True:
    line = cursor.read_significant()
    if line is None:
      break
    _line_limit(result, line, current)
    if line.raw.startswith(b"$"):
      found_file_end = True
      if line.text not in {"$", "$~"}:
        result.findings.append(
            finding(
                "HLQ015",
                "warning",
                f"noncanonical high-level quest terminator {line.text!r}; use '$~'",
                line.span,
            )
        )
      trailing = cursor.read_significant()
      if trailing is not None:
        result.findings.append(
            finding(
                "HLQ015",
                "error",
                "content after the high-level quest terminator is ignored by the loader",
                trailing.span,
            )
        )
        result.complete = False
      break

    if line.raw.startswith(b"#"):
      current = _parse_host_header(line, path, result)
      continue

    if line.text[:1] in {"A", "Q", "R"}:
      if current is None:
        result.findings.append(
            finding(
                "HLQ006",
                "error",
                f"{line.text[:1]} entry appears before a valid host mobile header; "
                "the loader dereferences an unsafe host pointer",
                line.span,
            )
        )
        result.complete = False
        _skip_orphan_entry(cursor, result, line)
        continue
      _parse_entry(cursor, result, current, line, entry_types, command_types)
      continue

    result.findings.append(
        finding(
            "HLQ007",
            "error",
            f"unknown top-level marker {line.text[:1]!r} truncates the remainder of the file",
            line.span,
            "hlquest" if current is not None else None,
            current.vnum if current is not None else None,
        )
    )
    _mark_incomplete(result, current)
    truncated = True
    break

  for record in result.records:
    _finalize_entry_order(record)
    _populate_references(record, manifest)
  if not found_file_end and not truncated:
    result.findings.append(
        finding(
            "HLQ014",
            "error",
            "high-level quest file is missing its conventional '$' terminator",
            SourceSpan(display_path, max(1, len(source.lines))),
        )
    )
    result.complete = False
  return result
