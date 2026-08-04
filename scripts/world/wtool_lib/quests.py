"""Byte-safe parser for Luminari QST flat files."""

from __future__ import annotations

from pathlib import Path
import re
from typing import Any

from .flags import decode_tokens
from .models import QuestRecord, SourceSpan, VnumReference
from .parsing import ParseResult, finding, source_issue_finding
from .source import (
    READ_SIZE,
    SourceCursor,
    SourceFile,
    parse_c_integer_prefix,
    parse_c_integer_token,
)


_HEADER = re.compile(r"^#([+-]?\d+)[ \t\v\f]*$")
_TOKEN = re.compile(r"\S+")
_INTEGER = re.compile(r"[ \t\v\f]*([+-]?\d+)")


def _line_limit(
    result: ParseResult[QuestRecord],
    line: Any,
    record: QuestRecord | None = None,
) -> None:
  if line.byte_length <= READ_SIZE - 2:
    return
  result.findings.append(
      finding(
          "QST001",
          "error",
          f"physical line is {line.byte_length} bytes; get_line() safely accepts at most "
          f"{READ_SIZE - 2}",
          line.span,
          "quest" if record is not None else None,
          record.vnum if record is not None else None,
      )
  )
  result.complete = False
  if record is not None:
    record.complete = False


def _span(line: Any, start: int, end: int) -> SourceSpan:
  return SourceSpan(
      line.span.path,
      line.span.line,
      start + 1,
      end_line=line.span.line,
      end_column=end + 1,
  )


def _record_error(
    result: ParseResult[QuestRecord],
    record: QuestRecord,
    code: str,
    message: str,
    span: SourceSpan,
) -> None:
  result.findings.append(finding(code, "error", message, span, "quest", record.vnum))
  result.complete = False
  record.complete = False


def _recover_record(cursor: SourceCursor) -> None:
  while not cursor.eof:
    line = cursor.peek_raw()
    if line is not None and (_HEADER.fullmatch(line.text) or line.raw.startswith(b"$")):
      return
    cursor.read_raw()


def _read_string(
    cursor: SourceCursor,
    result: ParseResult[QuestRecord],
    record: QuestRecord,
) -> tuple[str, SourceSpan, bool]:
  value = cursor.read_tilde_string()
  for issue in value.issues:
    parsed = source_issue_finding(issue, "QST", "quest", record.vnum)
    duplicate = any(
        existing.code == parsed.code
        and existing.span.path == parsed.span.path
        and existing.span.line == parsed.span.line
        for existing in result.findings
    )
    if not duplicate:
      result.findings.append(parsed)
    if issue.fatal:
      result.complete = False
      record.complete = False
  return value.text, value.span, value.terminated


def _store(
    record: QuestRecord,
    field_name: str,
    raw_value: int,
    span: SourceSpan,
    sentinel: bool = False,
) -> None:
  record.raw_values[field_name] = raw_value
  record.field_spans[field_name] = span
  setattr(record, field_name, None if sentinel and raw_value == -1 else raw_value)


def _scan_integer_row(
    line: Any,
    maximum: int,
) -> tuple[list[int], list[SourceSpan], int, str | None]:
  values: list[int] = []
  spans: list[SourceSpan] = []
  offset = 0
  for _ in range(maximum):
    match = _INTEGER.match(line.text, offset)
    if match is None:
      break
    parsed = parse_c_integer_prefix(line.text[offset:])
    if parsed.error is not None:
      return values, spans, offset + parsed.consumed, parsed.error
    assert parsed.value is not None
    values.append(parsed.value)
    spans.append(_span(line, match.start(1), match.end(1)))
    offset += parsed.consumed
    if offset < len(line.text) and not line.text[offset].isspace():
      break
  return values, spans, offset, None


def _parse_first_row(
    cursor: SourceCursor,
    result: ParseResult[QuestRecord],
    record: QuestRecord,
    manifest: dict[str, Any],
) -> bool:
  line = cursor.read_significant()
  if line is None:
    _record_error(result, record, "QST008", "quest ends before numeric row one", record.span)
    return False
  _line_limit(result, line, record)
  tokens = list(_TOKEN.finditer(line.text))
  if len(tokens) < 7:
    _record_error(
        result,
        record,
        "QST008",
        f"numeric row one has {len(tokens)} fields; the loader requires 7 conversions",
        line.span,
    )
    return False

  integer_positions = (0, 1, 3, 4, 5, 6)
  parsed_values: dict[int, int] = {}
  for position in integer_positions:
    token = tokens[position].group(0)
    parsed = (
        parse_c_integer_prefix(token)
        if position == integer_positions[-1]
        else parse_c_integer_token(token)
    )
    if parsed.error is not None or parsed.value is None:
      _record_error(
          result,
          record,
          "QST008",
          f"numeric row one field {position + 1} is invalid: {parsed.error}",
          _span(line, tokens[position].start(), tokens[position].end()),
      )
      return False
    parsed_values[position] = parsed.value

  ignored_start = (
      tokens[6].start() + parse_c_integer_prefix(tokens[6].group(0)).consumed
  )
  ignored = line.text[ignored_start:]
  if len(tokens) > 7 or ignored.strip():
    result.findings.append(
        finding(
            "QST008",
            "error",
            "numeric row one contains trailing data that sscanf() silently ignores",
            _span(line, ignored_start, len(line.text)),
            "quest",
            record.vnum,
        )
    )

  _store(record, "quest_type", parsed_values[0], _span(line, tokens[0].start(), tokens[0].end()))
  _store(
      record,
      "questmaster_vnum",
      parsed_values[1],
      _span(line, tokens[1].start(), tokens[1].end()),
      sentinel=True,
  )
  flag = tokens[2].group(0)
  record.flag_token = flag
  record.field_spans["flag_token"] = _span(line, tokens[2].start(), tokens[2].end())
  if len(flag.encode("utf-8", errors="surrogateescape")) > 127:
    _record_error(
        result,
        record,
        "QST009",
        "quest flag token exceeds the loader's 127-byte scanf field width and prevents "
        "the remaining numeric conversions",
        record.field_spans["flag_token"],
    )
    return False
  decoded = decode_tokens(
      [flag],
      entry_count=len(manifest["tables"]["quest"]["entries"]),
      serialized_chunks=1,
  )
  record.flag_bits = set(decoded.bits)
  for issue in decoded.issues:
    result.findings.append(
        finding(
            "QST009",
            "error",
            issue.message,
            record.field_spans["flag_token"],
            "quest",
            record.vnum,
        )
    )
  for field_name, position in (
      ("target", 3),
      ("previous_quest_vnum", 4),
      ("next_quest_vnum", 5),
      ("prerequisite_object_vnum", 6),
  ):
    _store(
        record,
        field_name,
        parsed_values[position],
        _span(line, tokens[position].start(), tokens[position].end()),
        sentinel=True,
    )
  return True


def _parse_numeric_row(
    cursor: SourceCursor,
    result: ParseResult[QuestRecord],
    record: QuestRecord,
    code: str,
    label: str,
    field_names: tuple[str, ...],
    allowed_counts: set[int],
    sentinel_fields: frozenset[str] = frozenset(),
) -> bool:
  line = cursor.read_significant()
  if line is None:
    _record_error(result, record, code, f"quest ends before {label}", record.span)
    return False
  _line_limit(result, line, record)
  values, spans, consumed, error = _scan_integer_row(line, len(field_names))
  if error is not None or len(values) not in allowed_counts:
    detail = f": {error}" if error is not None else ""
    _record_error(
        result,
        record,
        code,
        f"{label} has {len(values)} integer conversions; expected "
        f"{' or '.join(str(count) for count in sorted(allowed_counts))}{detail}",
        line.span,
    )
    return False

  if line.text[consumed:].strip():
    result.findings.append(
        finding(
            code,
            "error",
            f"{label} contains trailing data that sscanf() silently ignores",
            _span(line, consumed, len(line.text)),
            "quest",
            record.vnum,
        )
    )
  for field_name, value, span in zip(field_names, values, spans, strict=False):
    _store(record, field_name, value, span, sentinel=field_name in sentinel_fields)
  return True


def _parse_dialogue_block(
    cursor: SourceCursor,
    result: ParseResult[QuestRecord],
    record: QuestRecord,
    marker: Any,
) -> bool:
  if marker.text != "D":
    result.findings.append(
        finding(
            "QST012",
            "warning",
            f"noncanonical dialogue marker {marker.text!r}; trailing marker text is ignored",
            marker.span,
            "quest",
            record.vnum,
        )
    )
  if record.dialogue_block_count:
    result.findings.append(
        finding(
            "QST016",
            "error",
            "repeated D block overwrites the earlier dialogue values in memory",
            marker.span,
            "quest",
            record.vnum,
        )
    )
  record.dialogue_block_count += 1
  return _parse_numeric_row(
      cursor,
      result,
      record,
      "QST012",
      "dialogue row",
      ("diplomacy_dc", "intimidate_dc", "bluff_dc", "dialogue_alternative_quest_vnum"),
      {4},
      frozenset({"dialogue_alternative_quest_vnum"}),
  )


def _quest_type_values(manifest: dict[str, Any]) -> dict[str, int]:
  return {
      entry["macro"]: entry["index"]
      for entry in manifest["tables"]["quest-types"]["entries"]
      if entry.get("macro")
  }


def _add_reference(
    record: QuestRecord,
    target_type: str,
    target_vnum: int | None,
    role: str,
    field_name: str,
) -> None:
  if target_vnum is None:
    return
  record.references.append(
      VnumReference(
          target_type,
          target_vnum,
          role,
          record.field_spans.get(field_name, record.span),
      )
  )


def _populate_references(record: QuestRecord, manifest: dict[str, Any]) -> None:
  types = _quest_type_values(manifest)
  target_types = {
      types["AQ_OBJ_FIND"]: "object",
      types["AQ_OBJ_RETURN"]: "object",
      types["AQ_ROOM_FIND"]: "room",
      types["AQ_ROOM_CLEAR"]: "room",
      types["AQ_MOB_FIND"]: "mobile",
      types["AQ_MOB_KILL"]: "mobile",
      types["AQ_MOB_SAVE"]: "mobile",
      types["AQ_DIALOGUE"]: "mobile",
  }
  record.references.clear()
  _add_reference(
      record,
      "mobile",
      record.questmaster_vnum,
      "questmaster mobile",
      "questmaster_vnum",
  )
  if record.quest_type in target_types:
    _add_reference(
        record,
        target_types[record.quest_type],
        record.target,
        "type-sensitive quest target",
        "target",
    )
  if record.quest_type in {types["AQ_OBJ_RETURN"], types["AQ_GIVE_GOLD"]}:
    _add_reference(
        record,
        "mobile",
        record.return_mobile_vnum,
        "quest return recipient",
        "return_mobile_vnum",
    )
  for target_type, value, role, field_name in (
      (
          "object",
          record.prerequisite_object_vnum,
          "quest prerequisite object",
          "prerequisite_object_vnum",
      ),
      ("object", record.reward_object_vnum, "quest reward object", "reward_object_vnum"),
      ("mobile", record.follower_mobile_vnum, "quest follower reward", "follower_mobile_vnum"),
      ("quest", record.previous_quest_vnum, "previous quest", "previous_quest_vnum"),
      ("quest", record.next_quest_vnum, "next quest", "next_quest_vnum"),
      (
          "quest",
          record.dialogue_alternative_quest_vnum,
          "dialogue alternative quest",
          "dialogue_alternative_quest_vnum",
      ),
  ):
    _add_reference(record, target_type, value, role, field_name)


def parse_quest_file(
    path: Path,
    display_path: str,
    manifest: dict[str, Any],
) -> ParseResult[QuestRecord]:
  result: ParseResult[QuestRecord] = ParseResult()
  try:
    source = SourceFile.from_path(path, display_path)
  except OSError as error:
    result.findings.append(
        finding("QST004", "error", f"cannot read quest file: {error}", SourceSpan(display_path, 1))
    )
    result.complete = False
    return result

  for source_line in source.lines:
    if source_line.raw.startswith(b"*"):
      _line_limit(result, source_line)
  cursor = SourceCursor(source)
  found_file_end = False
  while True:
    header = cursor.read_significant()
    if header is None:
      break
    _line_limit(result, header)
    if header.raw.startswith(b"$"):
      found_file_end = True
      if header.text not in {"$", "$~"}:
        result.findings.append(
            finding(
                "QST017",
                "warning",
                f"noncanonical quest-file terminator {header.text!r}; use '$~'",
                header.span,
            )
        )
      trailing = cursor.read_significant()
      if trailing is not None:
        result.findings.append(
            finding(
                "QST017",
                "error",
                "content after the quest-file terminator is ignored by the loader",
                trailing.span,
            )
        )
        result.complete = False
      break

    match = _HEADER.fullmatch(header.text)
    if match is None:
      result.findings.append(
          finding("QST005", "error", "expected a #vnum quest header", header.span)
      )
      result.complete = False
      _recover_record(cursor)
      continue
    parsed_vnum = parse_c_integer_token(match.group(1))
    if parsed_vnum.error is not None or parsed_vnum.value is None:
      result.findings.append(
          finding("QST005", "error", f"invalid quest vnum: {parsed_vnum.error}", header.span)
      )
      result.complete = False
      _recover_record(cursor)
      continue

    record = QuestRecord(parsed_vnum.value, header.span, path.stem)
    record.raw_values["vnum"] = record.vnum
    record.field_spans["vnum"] = _span(header, 1, len(match.group(1)) + 1)
    result.records.append(record)
    if record.vnum < 0:
      result.findings.append(
          finding(
              "QST006",
              "error",
              "quest vnum must be non-negative",
              header.span,
              "quest",
              record.vnum,
          )
      )

    strings: list[str] = []
    string_names = (
        "name",
        "description",
        "accept_message",
        "completion_message",
        "quit_message",
    )
    strings_complete = True
    for field_name in string_names:
      value, span, complete = _read_string(cursor, result, record)
      record.field_spans[field_name] = span
      strings.append(value)
      if not complete:
        strings_complete = False
        break
    for field_name, value in zip(string_names, strings, strict=False):
      setattr(record, field_name, value)
    if not strings_complete:
      _recover_record(cursor)
      if cursor.eof:
        break
      continue

    if not _parse_first_row(cursor, result, record, manifest):
      _recover_record(cursor)
      continue
    if not _parse_numeric_row(
        cursor,
        result,
        record,
        "QST010",
        "numeric row two",
        (
            "points",
            "quit_penalty",
            "min_level",
            "max_level",
            "time_limit",
            "return_mobile_vnum",
            "quantity",
        ),
        {7},
        frozenset({"return_mobile_vnum"}),
    ):
      _recover_record(cursor)
      continue
    if not _parse_numeric_row(
        cursor,
        result,
        record,
        "QST011",
        "reward row",
        (
            "gold_reward",
            "experience_reward",
            "reward_object_vnum",
            "race_reward",
            "wilderness_x",
            "wilderness_y",
            "follower_mobile_vnum",
        ),
        {3, 7},
        frozenset({"reward_object_vnum", "race_reward", "follower_mobile_vnum"}),
    ):
      _recover_record(cursor)
      continue
    record.reward_row_width = sum(
        field_name in record.raw_values
        for field_name in (
            "gold_reward",
            "experience_reward",
            "reward_object_vnum",
            "race_reward",
            "wilderness_x",
            "wilderness_y",
            "follower_mobile_vnum",
        )
    )

    found_record_end = False
    while True:
      marker = cursor.read_significant()
      if marker is None:
        break
      _line_limit(result, marker, record)
      if _HEADER.fullmatch(marker.text) or marker.raw.startswith(b"$"):
        cursor.unread_raw()
        break
      kind = marker.text[:1]
      if kind == "D":
        if not _parse_dialogue_block(cursor, result, record, marker):
          _recover_record(cursor)
          break
      elif kind == "S":
        if marker.text != "S":
          result.findings.append(
              finding(
                  "QST014",
                  "error",
                  f"noncanonical record terminator {marker.text!r} truncates the record",
                  marker.span,
                  "quest",
                  record.vnum,
              )
          )
        found_record_end = True
        break
      else:
        _record_error(
            result,
            record,
            "QST013",
            f"unknown quest extension marker {kind!r}; expected D or S",
            marker.span,
        )
        _recover_record(cursor)
        break
    if not found_record_end:
      if record.complete:
        _record_error(
            result,
            record,
            "QST014",
            "quest is missing its S record terminator",
            record.span,
        )
      if cursor.eof:
        break

  if not found_file_end:
    result.findings.append(
        finding(
            "QST015",
            "error",
            "quest file is missing its conventional '$' terminator",
            SourceSpan(display_path, max(1, len(source.lines))),
        )
    )
    result.complete = False
  for record in result.records:
    _populate_references(record, manifest)
  return result
