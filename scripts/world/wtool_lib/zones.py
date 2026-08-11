"""Parser and structural checks for Luminari zone reset files."""

from __future__ import annotations

from pathlib import Path
import re
from typing import Any

from .flags import decode_tokens
from .models import ResetCommandRecord, SourceSpan, ZoneRecord
from .parsing import ParseResult, finding, scan_integers, strip_optional_tilde
from .source import READ_SIZE, SourceCursor, SourceFile, parse_c_integer_token


_HEADER_COUNTS = {4, 10, 11, 14}
_COMMANDS = frozenset("MOEGPDRITVJLFKXC")
_FLEX_FIVE = frozenset("MOEP")


def _line_limit(result: ParseResult[ZoneRecord], line: Any, vnum: int | None = None) -> None:
  if line.byte_length > READ_SIZE - 2:
    result.findings.append(
        finding(
            "ZON001",
            "error",
            f"physical line is {line.byte_length} bytes; get_line() safely accepts at most "
            f"{READ_SIZE - 2}",
            line.span,
            "zone" if vnum is not None else None,
            vnum,
        )
    )
    result.complete = False


def _parse_header_values(
    line: Any,
    result: ParseResult[ZoneRecord],
    vnum: int,
) -> list[Any] | None:
  tokens = line.text.split()
  if len(tokens) not in _HEADER_COUNTS:
    result.findings.append(
        finding(
            "ZON004",
            "error",
            f"zone numeric header has {len(tokens)} fields; expected exactly 4, 10, 11, or 14",
            line.span,
            "zone",
            vnum,
        )
    )
    return None

  integer_positions = {0, 1, 2, 3}
  if len(tokens) >= 10:
    integer_positions.update({8, 9})
  if len(tokens) >= 11:
    integer_positions.add(10)
  if len(tokens) == 14:
    integer_positions.update({11, 12, 13})
  values: list[Any] = list(tokens)
  for position in sorted(integer_positions):
    parsed = parse_c_integer_token(tokens[position])
    if parsed.error is not None:
      result.findings.append(
          finding(
              "ZON005",
              "error",
              f"invalid integer in zone header field {position + 1}: {parsed.error}",
              line.span,
              "zone",
              vnum,
          )
      )
      return None
    values[position] = parsed.value
  return values


def _range_finding(
    result: ParseResult[ZoneRecord],
    record: ZoneRecord,
    code: str,
    message: str,
    severity: str = "warning",
) -> None:
  result.findings.append(finding(code, severity, message, record.span, "zone", record.vnum))


def _validate_zone_header(
    result: ParseResult[ZoneRecord],
    record: ZoneRecord,
    manifest: dict[str, Any],
) -> None:
  assert record.bottom is not None and record.top is not None
  assert record.lifespan is not None and record.reset_mode is not None
  if record.vnum < 0:
    _range_finding(result, record, "ZON006", "zone vnum must be non-negative", "error")
  if record.bottom < 0 or record.top < 0:
    _range_finding(result, record, "ZON007", "zone room range must be non-negative", "error")
  if record.bottom > record.top:
    _range_finding(
        result,
        record,
        "ZON008",
        f"zone bottom {record.bottom} is greater than top {record.top}",
        "error",
    )
  if not 0 <= record.lifespan <= 240:
    _range_finding(
        result,
        record,
        "ZON009",
        f"lifespan {record.lifespan} is outside the OLC range 0..240",
    )
  if record.reset_mode not in {0, 1, 2}:
    _range_finding(
        result,
        record,
        "ZON010",
        f"reset mode {record.reset_mode} is outside the supported range 0..2",
        "error",
    )
  if record.min_level is not None and not -1 <= record.min_level <= 100:
    _range_finding(result, record, "ZON011", f"minimum level {record.min_level} is outside -1..100")
  if record.max_level is not None and not -1 <= record.max_level <= 100:
    _range_finding(result, record, "ZON012", f"maximum level {record.max_level} is outside -1..100")
  if (
      record.min_level is not None
      and record.max_level is not None
      and record.min_level >= 0
      and record.max_level >= 0
      and record.min_level > record.max_level
  ):
    _range_finding(result, record, "ZON013", "minimum level is greater than maximum level", "error")
  if record.show_weather not in {None, 0, 1}:
    _range_finding(
        result, record, "ZON014", f"show-weather value {record.show_weather} must be 0 or 1"
    )
  region_count = manifest.get("limits", {}).get("NUM_REGIONS", {}).get("value")
  city_count = manifest.get("limits", {}).get("NUM_CITIES", {}).get("value")
  faction_count = manifest.get("limits", {}).get("NUM_FACTIONS", {}).get("value")
  if region_count is not None and record.region is not None and not 0 <= record.region < region_count:
    _range_finding(
        result, record, "ZON015", f"region {record.region} is outside 0..{region_count - 1}", "error"
    )
  if city_count is not None and record.city is not None and not 0 <= record.city < city_count:
    _range_finding(
        result, record, "ZON016", f"city {record.city} is outside 0..{city_count - 1}", "error"
    )
  if (
      faction_count is not None
      and record.faction is not None
      and not 0 <= record.faction < faction_count
  ):
    _range_finding(
        result,
        record,
        "ZON017",
        f"faction {record.faction} is outside 0..{faction_count - 1}",
        "error",
    )


def _command_shape(command: str) -> tuple[int, int]:
  if command in _FLEX_FIVE:
    return 4, 5
  if command == "G":
    return 3, 4
  if command in {"D", "T"}:
    return 4, 4
  if command == "R":
    return 3, 4
  if command in {"I", "L"}:
    return 3, 3
  if command == "F":
    return 4, 5
  if command in {"K", "X", "C"}:
    return 5, 5
  if command == "J":
    return 2, 3
  raise ValueError(command)


def _parse_v_command(
    payload: str,
    line: Any,
    result: ParseResult[ZoneRecord],
    zone_vnum: int,
) -> ResetCommandRecord | None:
  values, offset, error = scan_integers(payload, 4)
  if error is not None or len(values) != 4:
    result.findings.append(
        finding(
            "ZON022",
            "error",
            "V reset requires four integers, a variable name, and a non-empty value",
            line.span,
            "zone",
            zone_vnum,
        )
    )
    return None
  remainder = payload[offset:].lstrip()
  match = re.match(r"([^\s]+)(?:[ \t\v\f]+)(.*)$", remainder)
  if match is None or not match.group(2):
    result.findings.append(
        finding(
            "ZON022",
            "error",
            "V reset requires a variable name followed by a non-empty value",
            line.span,
            "zone",
            zone_vnum,
        )
    )
    return None
  variable, value = match.groups()
  if re.fullmatch(r"[+-]?\d+", variable):
    result.findings.append(
        finding(
            "ZON023",
            "error",
            "V reset has the extra numeric placeholder emitted by save_zone(); the loader "
            "will use that number as the variable name and shift the intended strings",
            line.span,
            "zone",
            zone_vnum,
        )
    )
  if len(variable.encode("utf-8", errors="surrogateescape")) > 79:
    result.findings.append(
        finding("ZON024", "error", "V variable name exceeds the 79-byte loader limit", line.span, "zone", zone_vnum)
    )
  if len(value.encode("utf-8", errors="surrogateescape")) > 79 or any(
      character in value for character in "\f\r\t\v"
  ):
    result.findings.append(
        finding("ZON025", "error", "V value exceeds or violates the 79-byte loader field", line.span, "zone", zone_vnum)
    )
  dependency = values[0]
  return ResetCommandRecord("V", dependency, values[1:], [variable, value], line.span)


def _parse_command(
    line: Any,
    result: ParseResult[ZoneRecord],
    zone_vnum: int,
    direction_count: int,
    wear_count: int,
) -> ResetCommandRecord | None:
  command_text = line.text.lstrip()
  command = command_text[0]
  payload = command_text[1:].lstrip()
  if command == "V":
    parsed = _parse_v_command(payload, line, result, zone_vnum)
  else:
    minimum, maximum = _command_shape(command)
    values, _, error = scan_integers(payload, maximum)
    if error is not None or len(values) < minimum:
      result.findings.append(
          finding(
              "ZON021",
              "error",
              f"{command} reset has {len(values)} integer conversion(s); expected "
              f"{minimum}" + (f" or {maximum}" if minimum != maximum else ""),
              line.span,
              "zone",
              zone_vnum,
          )
      )
      return None
    dependency = values[0]
    arguments = values[1:]
    probability: int | None = None
    if command in _FLEX_FIVE:
      probability = arguments[3] if len(arguments) == 4 else 100
    elif command == "G":
      probability = arguments[2] if len(arguments) == 3 else 100
    elif command == "R":
      probability = arguments[2] if len(arguments) == 3 else 100
    elif command in {"K", "X"}:
      probability = arguments[3]
    elif command == "F":
      probability = arguments[3] if len(arguments) == 4 else 100
    elif command == "J":
      probability = arguments[1] if len(arguments) == 2 else 100
    elif command == "I":
      probability = arguments[0]
    elif command == "L":
      probability = arguments[1]
    parsed = ResetCommandRecord(
        command,
        dependency,
        arguments,
        [],
        line.span,
        probability=probability,
    )

  if parsed is None:
    return None
  if parsed.command != "F" and (parsed.dependency < -127 or parsed.dependency > 127):
    result.findings.append(
        finding(
            "ZON026",
            "error",
            f"dependency offset {parsed.dependency} is outside the usable signed queue range -127..127",
            line.span,
            "zone",
            zone_vnum,
        )
    )
  if parsed.probability is not None and not 0 <= parsed.probability <= 100:
    result.findings.append(
        finding(
            "ZON027",
            "warning",
            f"percentage {parsed.probability} is outside 0..100 and is silently changed by runtime behavior",
            line.span,
            "zone",
            zone_vnum,
        )
    )
  if parsed.command == "E" and len(parsed.arguments) >= 3:
    position = parsed.arguments[2]
    if not 0 <= position < wear_count:
      result.findings.append(
          finding(
              "ZON028",
              "error",
              f"equipment position {position} is outside 0..{wear_count - 1}",
              line.span,
              "zone",
              zone_vnum,
          )
      )
  if parsed.command == "D" and len(parsed.arguments) >= 3:
    direction, state = parsed.arguments[1], parsed.arguments[2]
    if not 0 <= direction < direction_count:
      result.findings.append(
          finding(
              "ZON029",
              "error",
              f"door direction {direction} is outside the effective range 0..{direction_count - 1}",
              line.span,
              "zone",
              zone_vnum,
          )
      )
    if not 0 <= state <= 16:
      result.findings.append(
          finding("ZON030", "error", f"door state {state} is outside 0..16", line.span, "zone", zone_vnum)
      )
  if parsed.command == "K" and len(parsed.arguments) >= 3:
    direction, state = parsed.arguments[1], parsed.arguments[2]
    if not 0 <= direction < direction_count:
      result.findings.append(
          finding(
              "ZON029",
              "error",
              f"door direction {direction} is outside the effective range 0..{direction_count - 1}",
              line.span,
              "zone",
              zone_vnum,
          )
      )
    if not 0 <= state <= 31:
      result.findings.append(
          finding(
              "ZON045",
              "error",
              f"legacy door bitmask {state} is outside 0..31",
              line.span,
              "zone",
              zone_vnum,
          )
      )
  if parsed.command == "F":
    if parsed.dependency not in {0, 1, 2, 3}:
      result.findings.append(
          finding(
              "ZON046",
              "error",
              f"follow mode {parsed.dependency} is outside 0..3",
              line.span,
              "zone",
              zone_vnum,
          )
      )
  if parsed.command == "C" and len(parsed.arguments) >= 4:
    hour, day, weekday, month = parsed.arguments[:4]
    if (
        not -1 <= hour <= 23
        or not 0 <= day <= 35
        or not 0 <= weekday <= 7
        or not 0 <= month <= 17
    ):
      result.findings.append(
          finding(
              "ZON047",
              "error",
              "calendar predicate is outside source hour/day/weekday/month ranges",
              line.span,
              "zone",
              zone_vnum,
          )
      )
  if parsed.command in {"T", "V"} and parsed.arguments:
    attach_type = parsed.arguments[0]
    if attach_type not in {0, 1, 2}:
      result.findings.append(
          finding("ZON031", "error", f"trigger host type {attach_type} is outside 0..2", line.span, "zone", zone_vnum)
      )
  if parsed.command == "J" and parsed.arguments and parsed.arguments[0] < 0:
    result.findings.append(
        finding("ZON032", "error", "jump count must be non-negative", line.span, "zone", zone_vnum)
    )
  if parsed.command == "I":
    result.findings.append(
        finding(
            "ZON033",
            "warning",
            "I reset requires a dummy third integer that the runtime ignores",
            line.span,
            "zone",
            zone_vnum,
        )
    )
  if parsed.command == "L":
    result.findings.append(
        finding(
            "ZON034",
            "error",
            "L reset is non-functional: the parser never initializes the container field used at runtime",
            line.span,
            "zone",
            zone_vnum,
        )
    )
  return parsed


def _host_finding(
    result: ParseResult[ZoneRecord],
    command: ResetCommandRecord,
    zone_vnum: int,
    subject: str,
    states: set[bool],
) -> None:
  if True not in states:
    severity = "error"
    message = f"{command.command} reset has no current {subject} host"
  elif False in states:
    severity = "warning"
    message = f"{command.command} reset may execute without a current {subject} host"
  else:
    return
  result.findings.append(finding("ZON037", severity, message, command.span, "zone", zone_vnum))


def _analyze_command_flow(result: ParseResult[ZoneRecord], record: ZoneRecord) -> None:
  queue_min = 0
  queue_max = 0
  possible_skips: set[int] = set()
  state: dict[str, set[bool]] = {
      "mob": {False},
      "tmob": {False},
      "tobj": {False},
      "tmob_script": {False},
      "tobj_script": {False},
  }

  for index, command in enumerate(record.commands):
    offset = abs(command.dependency)
    if command.command == "F":
      offset = 0
    if offset > queue_max:
      result.findings.append(
          finding(
              "ZON035",
              "error",
              f"dependency offset {command.dependency} cannot resolve; at most {queue_max} result(s) exist",
              command.span,
              "zone",
              record.vnum,
          )
      )
    elif offset > queue_min:
      result.findings.append(
          finding(
              "ZON036",
              "warning",
              f"dependency offset {command.dependency} is runtime-dependent because preceding commands "
              "do not always push a result",
              command.span,
              "zone",
              record.vnum,
          )
      )

    before = {name: set(values) for name, values in state.items()}
    push_min = push_max = 1
    if command.command == "M":
      state["mob"] = {True}
      state["tmob"] = {True}
      state["tobj"] = {False}
      state["tmob_script"] = {False}
      state["tobj_script"] = {False}
    elif command.command == "O":
      state["tmob"] = {False}
      state["tmob_script"] = {False}
      state["tobj"] = {True}
      state["tobj_script"] = {False}
      push_min, push_max = 0, 1
    elif command.command in {"P", "G", "E"}:
      if command.command in {"G", "E"}:
        _host_finding(result, command, record.vnum, "mobile", state["mob"])
      state["tmob"] = {False}
      state["tmob_script"] = {False}
      state["tobj"] = {True}
      state["tobj_script"] = {False}
    elif command.command == "I":
      _host_finding(result, command, record.vnum, "mobile", state["mob"])
      push_min, push_max = (0, 0) if state["mob"] == {True} else (0, 1)
    elif command.command in {"R", "D"}:
      state["tmob"] = {False}
      state["tobj"] = {False}
      state["tmob_script"] = {False}
      state["tobj_script"] = {False}
    elif command.command == "F":
      push_min = push_max = 0
    elif command.command in {"C", "K", "X"}:
      state["tmob"] = {False}
      state["tobj"] = {False}
      state["tmob_script"] = {False}
      state["tobj_script"] = {False}
    elif command.command == "T" and command.arguments:
      host_type = command.arguments[0]
      if host_type == 0:
        _host_finding(result, command, record.vnum, "mobile trigger", state["tmob"])
        if True in state["tmob"]:
          state["tmob_script"] = {True}
        push_min, push_max = ((1, 1) if state["tmob"] == {True} else (0, 1))
      elif host_type == 1:
        _host_finding(result, command, record.vnum, "object trigger", state["tobj"])
        if True in state["tobj"]:
          state["tobj_script"] = {True}
        push_min, push_max = ((1, 1) if state["tobj"] == {True} else (0, 1))
    elif command.command == "V" and command.arguments:
      host_type = command.arguments[0]
      if host_type == 0:
        _host_finding(result, command, record.vnum, "mobile variable", state["tmob"])
        _host_finding(result, command, record.vnum, "scripted mobile", state["tmob_script"])
        push_min, push_max = (0, 2)
      elif host_type == 1:
        _host_finding(result, command, record.vnum, "object variable", state["tobj"])
        _host_finding(result, command, record.vnum, "scripted object", state["tobj_script"])
        push_min, push_max = (0, 2)
      elif host_type == 2:
        push_min, push_max = (1, 2)

    may_not_execute = index in possible_skips or (
        command.command != "F" and command.dependency != 0
    )
    if may_not_execute:
      for name in state:
        state[name].update(before[name])
      push_min = min(push_min, 1)
      push_max = max(push_max, 1)

    queue_min = min(127, queue_min + push_min)
    queue_max = min(127, queue_max + push_max)
    if command.command == "J" and command.arguments and (command.probability or 0) > 0:
      count = max(0, command.arguments[0])
      possible_skips.update(range(index + 1, min(len(record.commands), index + 1 + count)))


def parse_zone_file(
    path: Path,
    display_path: str,
    manifest: dict[str, Any],
    direction_count: int,
) -> ParseResult[ZoneRecord]:
  result: ParseResult[ZoneRecord] = ParseResult()
  try:
    source = SourceFile.from_path(path, display_path)
  except OSError as error:
    result.findings.append(finding("ZON002", "error", f"cannot read zone file: {error}", SourceSpan(display_path, 1)))
    result.complete = False
    return result
  for source_line in source.lines:
    if source_line.raw.startswith(b"*"):
      _line_limit(result, source_line)
  cursor = SourceCursor(source)
  header = cursor.read_significant()
  if header is None:
    result.findings.append(finding("ZON003", "error", "zone file is empty", SourceSpan(display_path, 1)))
    result.complete = False
    return result
  _line_limit(result, header)
  match = re.match(r"^#([+-]?\d+)", header.text)
  if match is None:
    result.findings.append(finding("ZON003", "error", "expected a #vnum zone header", header.span))
    result.complete = False
    return result
  parsed_vnum = parse_c_integer_token(match.group(1))
  if parsed_vnum.error is not None or parsed_vnum.value is None:
    result.findings.append(finding("ZON003", "error", f"invalid zone vnum: {parsed_vnum.error}", header.span))
    result.complete = False
    return result
  vnum = parsed_vnum.value

  builders_line = cursor.read_significant()
  name_line = cursor.read_significant()
  numeric_line = cursor.read_significant()
  if builders_line is None or name_line is None or numeric_line is None:
    result.findings.append(finding("ZON002", "error", "zone header ends before all four fields", header.span, "zone", vnum))
    result.complete = False
    return result
  for line in (builders_line, name_line, numeric_line):
    _line_limit(result, line, vnum)
  values = _parse_header_values(numeric_line, result, vnum)
  if values is None:
    result.complete = False
    return result

  bottom, top, lifespan, reset_mode = values[:4]
  flag_tokens = ["0", "0", "0", "0"]
  min_level = max_level = -1
  show_weather = 1
  region = faction = city = 0
  if len(values) >= 10:
    flag_tokens = [str(value) for value in values[4:8]]
    min_level, max_level = values[8:10]
  if len(values) >= 11:
    show_weather = values[10]
  if len(values) == 14:
    region, faction, city = values[11:14]
  record = ZoneRecord(
      vnum=vnum,
      span=header.span,
      source_package=path.stem,
      name=strip_optional_tilde(name_line.text),
      builders=strip_optional_tilde(builders_line.text),
      bottom=bottom,
      top=top,
      lifespan=lifespan,
      reset_mode=reset_mode,
      flags=flag_tokens,
      min_level=min_level,
      max_level=max_level,
      show_weather=show_weather,
      region=region,
      faction=faction,
      city=city,
      header_field_count=len(values),
  )
  result.records.append(record)
  _validate_zone_header(result, record, manifest)
  zone_entries = manifest["tables"]["zone"]["entries"]
  decoded = decode_tokens(flag_tokens, entry_count=len(zone_entries))
  for issue in decoded.issues:
    result.findings.append(finding(issue.code, "error", issue.message, numeric_line.span, "zone", vnum))

  wear_count = manifest["limits"].get("NUM_WEARS", {}).get("value", 43)
  found_sentinel = False
  while True:
    line = cursor.read_significant()
    if line is None:
      break
    _line_limit(result, line, vnum)
    stripped = line.text.lstrip()
    if stripped.startswith("*"):
      continue
    if stripped.startswith("S"):
      if line.text != "S":
        result.findings.append(
            finding(
                "ZON018",
                "error",
                "zone sentinel must be exactly 'S' in column zero for the prescan and parser to agree",
                line.span,
                "zone",
                vnum,
            )
        )
      found_sentinel = True
      break
    command = stripped[:1]
    if command not in _COMMANDS:
      result.findings.append(
          finding("ZON019", "error", f"unknown or lowercase reset command {command!r}", line.span, "zone", vnum)
      )
      continue
    if line.text[0] != command or len(line.text) < 2 or line.text[1] != " ":
      result.findings.append(
          finding(
              "ZON020",
              "error",
              "reset command must start in column zero and use a literal space as byte two",
              line.span,
              "zone",
              vnum,
          )
      )
    parsed = _parse_command(line, result, vnum, direction_count, wear_count)
    if parsed is not None:
      record.commands.append(parsed)

  if not found_sentinel:
    result.findings.append(finding("ZON038", "error", "zone is missing its S command sentinel", header.span, "zone", vnum))
    result.complete = False
  else:
    terminator = cursor.read_significant()
    if terminator is None or not terminator.text.startswith("$"):
      span = terminator.span if terminator is not None else header.span
      result.findings.append(finding("ZON039", "error", "zone file is missing its conventional '$' terminator", span, "zone", vnum))
      result.complete = False
    else:
      trailing = cursor.read_significant()
      if trailing is not None:
        result.findings.append(
            finding(
                "ZON044",
                "error",
                "zone file contains data after its '$' terminator; exactly one zone record is allowed",
                trailing.span,
                "zone",
                vnum,
            )
        )
        result.complete = False
  _analyze_command_flow(result, record)
  return result
