"""Parser for Luminari room files and their bounded extension records."""

from __future__ import annotations

from pathlib import Path
import re
from typing import Any

from .flags import decode_tokens
from .models import (
    AttachmentRecord,
    ExitRecord,
    ExtraDescriptionRecord,
    MovingConnectionRecord,
    MovingRoomRecord,
    RolExitTrapRecord,
    RoomRecord,
    SourceSpan,
)
from .parsing import ParseResult, finding, scan_integers, source_issue_finding
from .source import READ_SIZE, SourceCursor, SourceFile, parse_c_integer_token


def _line_limit(result: ParseResult[RoomRecord], line: Any, vnum: int | None = None) -> None:
  if line.byte_length > READ_SIZE - 2:
    result.findings.append(
        finding(
            "WLD001",
            "error",
            f"physical line is {line.byte_length} bytes; get_line() safely accepts at most "
            f"{READ_SIZE - 2}",
            line.span,
            "room" if vnum is not None else None,
            vnum,
        )
    )
    result.complete = False


def _moving_line_limit(result: ParseResult[RoomRecord], line: Any, vnum: int) -> None:
  if line.byte_length > 255:
    result.findings.append(
        finding(
            "WLD034",
            "error",
            f"moving-room line is {line.byte_length} bytes; the 256-byte source buffer safely "
            "holds at most 255 bytes after get_line() removes the newline",
            line.span,
            "room",
            vnum,
        )
    )


def _read_string(
    cursor: SourceCursor,
    result: ParseResult[RoomRecord],
    vnum: int,
) -> tuple[str, SourceSpan, bool]:
  value = cursor.read_tilde_string()
  for issue in value.issues:
    parsed_finding = source_issue_finding(issue, "WLD", "room", vnum)
    duplicate = any(
        existing.code == parsed_finding.code
        and existing.span.path == parsed_finding.span.path
        and existing.span.line == parsed_finding.span.line
        for existing in result.findings
    )
    if not duplicate:
      result.findings.append(parsed_finding)
    if issue.fatal:
      result.complete = False
  return value.text, value.span, value.terminated


def _parse_exit(
    cursor: SourceCursor,
    line: Any,
    room: RoomRecord,
    result: ParseResult[RoomRecord],
    direction_limit: int,
    diagonal_dirs: bool,
) -> bool:
  match = re.match(r"^D\s*([+-]?\d+)", line.text)
  direction = int(match.group(1)) if match is not None else -1
  if match is None or not 0 <= direction < direction_limit:
    result.findings.append(
        finding(
            "WLD010",
            "error",
            f"exit direction {direction if match else line.text[1:]!r} is outside 0..{direction_limit - 1}",
            line.span,
            "room",
            room.vnum,
        )
    )
  if 6 <= direction <= 9 and not diagonal_dirs:
    result.findings.append(
        finding(
            "WLD011",
            "error",
            "diagonal exit is present while diagonal_dirs is disabled; setup_dir() returns "
            "without consuming the exit block",
            line.span,
            "room",
            room.vnum,
        )
    )
  description, _, description_complete = _read_string(cursor, result, room.vnum)
  if not description_complete:
    room.complete = False
    return False
  keyword, _, keyword_complete = _read_string(cursor, result, room.vnum)
  if not keyword_complete:
    room.complete = False
    return False
  numeric = cursor.read_significant()
  if numeric is None:
    result.findings.append(
        finding("WLD012", "error", "exit block ends before its three integers", line.span, "room", room.vnum)
    )
    room.complete = False
    result.complete = False
    return False
  _line_limit(result, numeric, room.vnum)
  values, _, error = scan_integers(numeric.text, 3)
  if error is not None or len(values) != 3:
    result.findings.append(
        finding("WLD012", "error", "exit block requires three integers", numeric.span, "room", room.vnum)
    )
    return True
  door_flags, key_vnum, destination = values
  if door_flags not in set(range(9)):
    result.findings.append(
        finding(
            "WLD013",
            "warning",
            f"door flag {door_flags} is silently treated as no door",
            numeric.span,
            "room",
            room.vnum,
        )
    )
  if key_vnum in {-1, 65535}:
    key_vnum = -1
  elif key_vnum < 0:
    result.findings.append(
        finding("WLD014", "error", f"invalid negative key vnum {key_vnum}", numeric.span, "room", room.vnum)
    )
  if destination in {-1, 0, 65535}:
    destination = -1
  elif destination < 0:
    result.findings.append(
        finding(
            "WLD015",
            "error",
            f"invalid negative exit destination {destination}",
            numeric.span,
            "room",
            room.vnum,
        )
    )
  if any(existing.direction == direction for existing in room.exits):
    result.findings.append(
        finding(
            "SEM002",
            "warning",
            f"room defines direction {direction} more than once; the later exit overwrites the earlier one",
            line.span,
            "room",
            room.vnum,
        )
    )
  room.exits.append(
      ExitRecord(
          direction,
          description or None,
          keyword or None,
          door_flags,
          key_vnum,
          destination,
          line.span,
      )
  )
  return True


def _parse_moving_room(
    cursor: SourceCursor,
    line: Any,
    room: RoomRecord,
    result: ParseResult[RoomRecord],
    direction_limit: int,
    max_connections: int,
) -> bool:
  values, _, error = scan_integers(line.text[1:], 5)
  if error is not None or len(values) != 5:
    result.findings.append(
        finding("WLD020", "error", "moving-room header requires five integers", line.span, "room", room.vnum)
    )
    values = (values + [0, 0, 0, 0, 0])[:5]
  inbound, pulses, random_move, exit_info, key_vnum = values
  if not 0 <= inbound < direction_limit:
    result.findings.append(
        finding(
            "WLD021",
            "error",
            f"moving-room inbound direction {inbound} is outside 0..{direction_limit - 1}",
            line.span,
            "room",
            room.vnum,
        )
    )

  messages: list[str | None] = []
  for label in ("transit", "docking", "destination docking"):
    message = cursor.read_significant()
    if message is None:
      result.findings.append(
          finding("WLD022", "error", f"moving room is missing its {label} message", line.span, "room", room.vnum)
      )
      messages.append(None)
      result.complete = False
      room.complete = False
      return False
    _line_limit(result, message, room.vnum)
    _moving_line_limit(result, message, room.vnum)
    if message.byte_length > 199:
      result.findings.append(
          finding(
              "WLD023",
              "warning",
              f"moving-room {label} message is truncated to 199 bytes",
              message.span,
              "room",
              room.vnum,
          )
      )
    messages.append(None if message.text.startswith("~") else message.text)

  connections: list[MovingConnectionRecord] = []
  expanded_count = 0
  found_end = False
  while True:
    connection = cursor.read_significant()
    if connection is None:
      break
    _line_limit(result, connection, room.vnum)
    _moving_line_limit(result, connection, room.vnum)
    if connection.text.startswith("~"):
      found_end = True
      break
    conn_values, _, conn_error = scan_integers(connection.text, 3)
    if conn_error is not None or len(conn_values) != 3:
      result.findings.append(
          finding("WLD024", "error", "moving-room connection requires three integers", connection.span, "room", room.vnum)
      )
      continue
    target, direction, repeat = conn_values
    effective_repeat = min(50, max(1, repeat))
    if effective_repeat != repeat:
      result.findings.append(
          finding(
              "WLD025",
              "warning",
              f"moving-room repeat {repeat} is silently clamped to {effective_repeat}",
              connection.span,
              "room",
              room.vnum,
          )
      )
    if not 0 <= direction < direction_limit:
      result.findings.append(
          finding(
              "WLD026",
              "error",
              f"moving-room direction {direction} is outside 0..{direction_limit - 1}",
              connection.span,
              "room",
              room.vnum,
          )
      )
    expanded_count += effective_repeat
    connections.append(MovingConnectionRecord(target, direction, effective_repeat, connection.span))
  if not found_end:
    result.findings.append(
        finding("WLD027", "error", "moving-room connection list is missing its '~' sentinel", line.span, "room", room.vnum)
    )
    result.complete = False
    room.complete = False
    return False
  if expanded_count >= max_connections:
    result.findings.append(
        finding(
            "WLD028",
            "error",
            f"moving-room expands to {expanded_count} connections; the source stores strictly fewer "
            f"than {max_connections} safely",
            line.span,
            "room",
            room.vnum,
        )
    )
  room.moving_connections.extend(connections)
  room.moving_room = MovingRoomRecord(
      inbound,
      pulses,
      random_move,
      exit_info,
      key_vnum,
      messages,
      connections,
      line.span,
  )
  return True


def _read_attachments(
    cursor: SourceCursor,
    room: RoomRecord,
    result: ParseResult[RoomRecord],
) -> None:
  while True:
    line = cursor.peek_raw()
    if line is None:
      return
    stripped = line.text.lstrip()
    if not stripped:
      cursor.read_raw()
      continue
    if not stripped.startswith("T"):
      return
    cursor.read_raw()
    _line_limit(result, line, room.vnum)
    tokens = stripped.split()
    if len(tokens) < 2:
      result.findings.append(
          finding("WLD030", "error", "inline trigger attachment requires a trigger vnum", line.span, "room", room.vnum)
      )
      continue
    parsed = parse_c_integer_token(tokens[1])
    if parsed.error is not None or parsed.value is None:
      result.findings.append(
          finding("WLD030", "error", f"invalid inline trigger vnum: {parsed.error}", line.span, "room", room.vnum)
      )
      continue
    room.attachments.append(AttachmentRecord(parsed.value, "room", room.vnum, line.span))


def _recover_record(cursor: SourceCursor) -> None:
  while not cursor.eof:
    line = cursor.peek_raw()
    if line is not None and (line.text.startswith("#") or line.text.startswith("$")):
      return
    cursor.read_raw()


def parse_room_file(
    path: Path,
    display_path: str,
    manifest: dict[str, Any],
    diagonal_dirs: bool,
    spec_names: set[str],
) -> ParseResult[RoomRecord]:
  result: ParseResult[RoomRecord] = ParseResult()
  try:
    source = SourceFile.from_path(path, display_path)
  except OSError as error:
    result.findings.append(finding("WLD004", "error", f"cannot read room file: {error}", SourceSpan(display_path, 1)))
    result.complete = False
    return result
  for source_line in source.lines:
    if source_line.raw.startswith(b"*"):
      _line_limit(result, source_line)
  cursor = SourceCursor(source)
  direction_limit = len(manifest["tables"]["directions"]["entries"])
  max_connections = manifest["limits"]["MAX_MOVING_ROOMS"]["value"]
  room_flag_count = len(manifest["tables"]["room"]["entries"])

  found_file_end = False
  while True:
    header = cursor.read_significant()
    if header is None:
      break
    _line_limit(result, header)
    if header.text.startswith("$"):
      found_file_end = True
      break
    match = re.match(r"^#([+-]?\d+)", header.text)
    if match is None:
      result.findings.append(finding("WLD005", "error", "expected a #vnum room header", header.span))
      _recover_record(cursor)
      continue
    parsed_vnum = parse_c_integer_token(match.group(1))
    if parsed_vnum.error is not None or parsed_vnum.value is None:
      result.findings.append(finding("WLD005", "error", f"invalid room vnum: {parsed_vnum.error}", header.span))
      _recover_record(cursor)
      continue
    vnum = parsed_vnum.value
    room = RoomRecord(vnum, header.span, path.stem)
    result.records.append(room)
    if vnum < 0:
      result.findings.append(finding("WLD006", "error", "room vnum must be non-negative", header.span, "room", vnum))

    room.name, _, name_complete = _read_string(cursor, result, vnum)
    if not name_complete:
      room.complete = False
      return result
    room.description, _, description_complete = _read_string(cursor, result, vnum)
    if not description_complete:
      room.complete = False
      return result
    flags_line = cursor.read_significant()
    if flags_line is None:
      result.findings.append(
          finding("WLD007", "error", "room ends before its flags and sector line", header.span, "room", vnum)
      )
      room.complete = False
      result.complete = False
      return result
    _line_limit(result, flags_line, vnum)
    tokens = flags_line.text.split()
    if len(tokens) not in {3, 6} and len(tokens) < 6:
      result.findings.append(
          finding(
              "WLD007",
              "error",
              f"room flags line has {len(tokens)} conversions; expected modern 6-field or "
              "legacy 3-field form",
              flags_line.span,
              "room",
              vnum,
          )
      )
      room.complete = False
      _recover_record(cursor)
      continue
    file_zone = parse_c_integer_token(tokens[0])
    sector_token = tokens[2] if len(tokens) == 3 else tokens[5]
    sector = parse_c_integer_token(sector_token)
    if file_zone.error is not None or sector.error is not None or file_zone.value is None or sector.value is None:
      result.findings.append(
          finding("WLD008", "error", "invalid integer on room flags line", flags_line.span, "room", vnum)
      )
      room.complete = False
      _recover_record(cursor)
      continue
    room.file_zone = file_zone.value
    room.sector = sector.value
    if len(tokens) == 3:
      room.flags = [tokens[1], "0", "0", "0"]
      result.findings.append(
          finding(
              "WLD009",
              "warning",
              "legacy 3-field room flags are accepted but will be converted when saved",
              flags_line.span,
              "room",
              vnum,
          )
      )
    else:
      room.flags = tokens[1:5]
    decoded = decode_tokens(room.flags, entry_count=room_flag_count)
    for issue in decoded.issues:
      result.findings.append(finding(issue.code, "error", issue.message, flags_line.span, "room", vnum))
    sector_count = len(manifest["tables"]["sectors"]["entries"])
    if not 0 <= room.sector < sector_count:
      result.findings.append(
          finding(
              "SEM001",
              "error",
              f"sector {room.sector} is outside 0..{sector_count - 1}",
              flags_line.span,
              "room",
              vnum,
          )
      )

    found_room_end = False
    level_range_seen = False
    rol_exit_trap_directions: set[int] = set()
    while True:
      line = cursor.read_significant()
      if line is None:
        break
      _line_limit(result, line, vnum)
      kind = line.text[:1]
      if kind in {"#", "$"}:
        cursor.unread_raw()
        break
      if kind == "C":
        coordinates = cursor.read_significant()
        if coordinates is None:
          result.findings.append(
              finding("WLD016", "error", "coordinate record is missing its numeric line", line.span, "room", vnum)
          )
          room.complete = False
          result.complete = False
          return result
        _line_limit(result, coordinates, vnum)
        values, _, error = scan_integers(coordinates.text, 2)
        if error is not None or len(values) != 2:
          result.findings.append(
              finding("WLD016", "error", "coordinate record requires two integers", coordinates.span, "room", vnum)
          )
        else:
          room.coordinates = (values[0], values[1])
      elif kind == "D":
        if not _parse_exit(cursor, line, room, result, direction_limit, diagonal_dirs):
          return result
      elif kind == "E":
        keywords, keyword_span, keywords_complete = _read_string(cursor, result, vnum)
        if not keywords_complete:
          room.complete = False
          return result
        description, _, extra_complete = _read_string(cursor, result, vnum)
        if not extra_complete:
          room.complete = False
          return result
        room.extra_descriptions.append(
            ExtraDescriptionRecord(keywords or None, description or None, keyword_span)
        )
      elif kind == "R":
        values, consumed, error = scan_integers(line.text[1:], 2)
        trailing = line.text[1:][consumed:].strip()
        if error is not None or len(values) != 2 or trailing:
          result.findings.append(
              finding(
                  "WLD036",
                  "error",
                  "room level-range R record requires exactly two integers",
                  line.span,
                  "room",
                  vnum,
              )
          )
        elif level_range_seen:
          result.findings.append(
              finding(
                  "WLD036",
                  "error",
                  "room has more than one level-range R record",
                  line.span,
                  "room",
                  vnum,
              )
          )
        else:
          room.minimum_level, room.maximum_level = values
          level_range_seen = True
      elif kind == "Y":
        values, consumed, error = scan_integers(line.text[1:], 8)
        trailing = line.text[1:][consumed:].strip()
        if error is not None or len(values) != 8 or trailing:
          result.findings.append(
              finding(
                  "WLD037",
                  "error",
                  "RoL exit-trap Y record requires exactly eight integers",
                  line.span,
                  "room",
                  vnum,
              )
          )
          continue
        direction, state, trap_type, minimum, maximum, area, hardness, load = values
        if (
            direction < 0
            or direction >= direction_limit
            or state not in {0, 1}
            or trap_type not in {1, 2, 3, 4, 5, 10, 11}
            or minimum < 0
            or maximum < minimum
            or maximum > 32766
            or area not in {0, 1}
            or hardness < -100
            or hardness > 100
            or load < 0
            or load > 100
        ):
          result.findings.append(
              finding(
                  "WLD038",
                  "error",
                  "RoL exit-trap Y values are outside their supported bounds",
                  line.span,
                  "room",
                  vnum,
              )
          )
          continue
        if direction in rol_exit_trap_directions:
          result.findings.append(
              finding(
                  "WLD039",
                  "error",
                  f"room has more than one RoL exit trap for direction {direction}",
                  line.span,
                  "room",
                  vnum,
              )
          )
          continue
        rol_exit_trap_directions.add(direction)
        room.rol_exit_traps.append(
            RolExitTrapRecord(
                direction,
                state,
                trap_type,
                minimum,
                maximum,
                area,
                hardness,
                load,
                line.span,
            )
        )
      elif kind == "M":
        if room.moving_room is not None:
          result.findings.append(
              finding(
                  "WLD035",
                  "error",
                  "room has more than one M record; setup_moving_room() returns without "
                  "consuming the duplicate block",
                  line.span,
                  "room",
                  vnum,
              )
          )
        if not _parse_moving_room(
            cursor, line, room, result, direction_limit, max_connections
        ):
          return result
      elif kind == "Z":
        spec_line = cursor.read_significant()
        if spec_line is None:
          result.findings.append(
              finding("WLD029", "error", "Z record is missing its spec-proc name", line.span, "room", vnum)
          )
          room.complete = False
          result.complete = False
          return result
        _line_limit(result, spec_line, vnum)
        room.spec_proc = spec_line.text
        if spec_line.text.casefold() not in spec_names:
          result.findings.append(
              finding(
                  "WLD029",
                  "error",
                  f"unknown persisted spec-proc name {spec_line.text!r}",
                  spec_line.span,
                  "room",
                  vnum,
              )
          )
      elif kind == "S":
        found_room_end = True
        _read_attachments(cursor, room, result)
        break
      else:
        result.findings.append(
            finding(
                "WLD031",
                "error",
                f"unknown room extension record {kind!r}; expected C, D, E, M, R, Y, Z, or S",
                line.span,
                "room",
                vnum,
            )
        )
        _recover_record(cursor)
        break
    if not found_room_end:
      result.findings.append(
          finding("WLD032", "error", "room is missing its S record terminator", header.span, "room", vnum)
      )
      room.complete = False
      result.complete = False
      if cursor.eof:
        return result

  if not found_file_end:
    result.findings.append(
        finding("WLD033", "error", "room file is missing its conventional '$' terminator", SourceSpan(display_path, max(1, len(source.lines))))
    )
    result.complete = False
  return result
